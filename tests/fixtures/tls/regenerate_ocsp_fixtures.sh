#!/usr/bin/env bash
# Regenererer OCSP/CRL-fixturane som ei konsistent CA-hierarki:
#   localhost.crt (CA, CA:TRUE) signerer ocsp-server.crt og revoked-client.crt (leaves),
#   og signerer OCSP-responsane (ocsp-good/-revoked.der) + CRL-en.
# Grunn: dei gamle fixturane var self-signa med responder != issuer, så OpenSSL
# (og den native OpenSSL-3-backenden) avviste dei ("unverified OCSP response").
# Klienten i test_native_tls_event_loop.no stolar på localhost.crt (CA), så denne
# strukturen verifiserer utan test-endringar.
set -euo pipefail
cd "$(dirname "$0")"

CA_CRT=localhost.crt
CA_KEY=localhost.key
OCSP_SERVER_SERIAL=0x1001      # held eksisterande serial
REVOKED_SERIAL=0x1000

# --- ocsp-server.crt: leaf signert av localhost CA (held eksisterande nøkkel) ---
openssl req -new -key ocsp-server.key -subj "/CN=localhost" -out /tmp/ocsp-server.csr
openssl x509 -req -in /tmp/ocsp-server.csr -CA "$CA_CRT" -CAkey "$CA_KEY" \
  -set_serial "$OCSP_SERVER_SERIAL" -days 3650 \
  -extfile <(printf "subjectAltName=DNS:localhost\nextendedKeyUsage=serverAuth\n") \
  -out ocsp-server.crt

# --- revoked-client.crt: leaf signert av localhost CA (server+client-bruk) ---
openssl req -new -key revoked-client.key -subj "/CN=norscode-client" -out /tmp/revoked-client.csr
openssl x509 -req -in /tmp/revoked-client.csr -CA "$CA_CRT" -CAkey "$CA_KEY" \
  -set_serial "$REVOKED_SERIAL" -days 3650 \
  -extfile <(printf "subjectAltName=DNS:norscode-client,DNS:localhost\nextendedKeyUsage=serverAuth,clientAuth\n") \
  -out revoked-client.crt

# --- OCSP index-filer (V=valid, R=revoked) ---
EXPIRY=360101000000Z
printf "V\t%s\t\t%s\tunknown\t/CN=localhost\n" "$EXPIRY" "1001" > /tmp/index-good.txt
printf "R\t%s\t%s\t%s\tunknown\t/CN=norscode-client\n" "$EXPIRY" "260101000000Z" "1000" > /tmp/index-revoked.txt

# --- OCSP-responsar signert av localhost CA (issuer=responder=CA) ---
openssl ocsp -issuer "$CA_CRT" -cert ocsp-server.crt -reqout /tmp/req-good.der -no_nonce
openssl ocsp -index /tmp/index-good.txt -CA "$CA_CRT" -rsigner "$CA_CRT" -rkey "$CA_KEY" \
  -reqin /tmp/req-good.der -respout ocsp-good.der -ndays 3650 -no_nonce

openssl ocsp -issuer "$CA_CRT" -cert revoked-client.crt -reqout /tmp/req-revoked.der -no_nonce
openssl ocsp -index /tmp/index-revoked.txt -CA "$CA_CRT" -rsigner "$CA_CRT" -rkey "$CA_KEY" \
  -reqin /tmp/req-revoked.der -respout ocsp-revoked.der -ndays 3650 -no_nonce

# --- CRL som revokerer revoked-client (serial 1000), signert av localhost CA ---
cat > /tmp/crl.cnf <<EOF
[ca]
default_ca = CA_default
[CA_default]
database = /tmp/crl-index.txt
default_md = sha256
crl_extensions = crl_ext
default_crl_days = 3650
[crl_ext]
authorityKeyIdentifier=keyid:always
EOF
printf "R\t%s\t%s\t%s\tunknown\t/CN=norscode-client\n" "$EXPIRY" "260101000000Z" "1000" > /tmp/crl-index.txt
openssl ca -config /tmp/crl.cnf -gencrl -cert "$CA_CRT" -keyfile "$CA_KEY" -out revoked-client.crl.pem

echo "=== ferdig regenerert ==="
