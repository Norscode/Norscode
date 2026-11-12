# TLS-tillit og sikker entropi

Den reine Norscode-transporten krev eit eksplisitt tillitsanker. Tom CA,
uleseleg fil, ugyldig PEM/X.509 eller manglande vertsnamn blir avvist før
klientsokkelen blir opna. Transporten bruker ikkje automatisk eit OS-lager.

## Konfigurer HTTPS-klientar

Vel ein PEM-fil med CA-sertifikat eller eit eksplisitt pinna bladsertifikat:

```sh
NORSCODE_TLS_CA_FILE=/absolutt/sti/til/ca-bundle.pem ./bin/nc run app.no
```

Dette gjeld òg `std.http.request`, `std.http.request_full` og Quick3, som gir
tomt `ca_file` til transporten. Eit ikkje-tomt `ca_file` i socket-API-et eller
`std.http.request_full_ca` har prioritet framfor miljøvariabelen. Ei feil i det
eksplisitte argumentet utløyser feil; transporten prøver ikkje eit anna anker.
Tilgang til miljøvariabelen og fila må vere tillaten i runtime-kapasitetane.

Klienten kontrollerer heile den mottatte sertifikatkjeda, gyldigheitstid,
vertsnamn i Subject Alternative Name (SAN), CertificateVerify og Finished. Utferdarsertifikat må ha CA=true,
keyCertSign når keyUsage finst, og tilstrekkeleg pathLen. Eit pinna
bladsertifikat må framleis ha gyldig dato og rett vertsnamn.
Sertifikat med berre CN blir avvist, også i `std.tls13_klient`/Postgres-ruta.
CN blir ikkje henta frå den rendra subject-strengen; DN-verdiar kan innehalde
tekst som `,CN=...` utan at det er eit nytt identitetsfelt.
`peer_verified` blir ikkje sett før tillit og nøkkelbevis er godkjende; ein
mislykka sesjon kan ikkje takast opp att som ein etablert sesjon.
Lesing, skriving og app-krypto krev eit fullført handtrykk, også når tenaren
allereie har avleidd appnøklar og enno ventar på klient-Finished/mTLS.

## Revokasjonskrav er avviste

`crl_file`, `client_crl_file` og `require_ocsp` gir ein tydeleg
`verification unsupported`-feil. Dei blir ikkje ignorerte. CRL-signatur åleine
beviser ikkje rett CRL-type, gyldig tidsrom og dekning. OCSP sitt `certStatus`
åleine beviser heller ikkje signatur, rett CertID eller fersk respons.
Komplett verifikasjon av desse policyane er enno ikkje implementert.

Valfri OCSP-stapling og felta `ocsp_stapled`/`ocsp_status` kan framleis vise
mottatte data, men er uverifisert metadata. Dei er ikkje revokasjonsbevis.
Tidlegare testar som godtok eit «good»-felt eller delvis CRL-kontroll skal
derfor no forvente policyfeil ved oppsett. Brukarar som krev revokasjonskontroll
kan ikkje etablere samband gjennom denne backend-en før støtta er komplett.

## Isolert rånøkkel-testmodus

Byte-inn/byte-ut-API-et i `std.tls13_session` har ei eksplisitt testform:
klienten gir `test_raw_peer_key` med nøyaktig 32 pinna Ed25519-byte,
vertsnamnet `localhost`, `127.0.0.1` eller `::1`, og ingen CA. Tenaren må gi
`test_raw_key: sann` når han bruker rå identitetsnøkkel utan X.509. Ein annan
nøkkel blir avvist. CertificateVerify og Finished er framleis obligatoriske.

Nettverks-ABI-et vidaresender ikkje desse opsjonane. Dei gir ingen generell
«insecure»-modus og kan ikkje aktiverast gjennom ei miljøvariabel.

## Tokens

`std.krypto.token`, `token_url`, `token_sifre`, `uuid4` og PKCE-verifier bruker
runtime-en si sikre OS-entropi. Feil frå entropikjelda blir kasta vidare; det
finst ingen fallback til klokke eller teljar. `token_url(n)` returnerer
base64url utan padding av n tilfeldige byte. `token_sifre(n)` bruker avvising
av byte 250–255 før modulo 10, slik at alle siffer har same sannsyn.
TLS ClientHello og ServerHello får òg sikre tilfeldige byte; den gamle
deterministiske nullverdien er fjerna.

## Verifikasjon og innebygde runtime-ruter

- `test_tls13_session_trust.no`: ugyldig/manglande tillit, dato, vertsnamn og
  avgrensa rånøkkelmodus gjennom Certificate-behandlinga.
- `test_tls13_network_trust.no`: CA-filer, miljøkonfigurasjon og avvising før I/O.
- `test_tls13_public_trust.no` (Linux x86-64): ferske kjeldebuntar og native-bygg
  av begge offentlege transportfixturane, med null uløyste kall og ekte køyring.
- `test_native_tls_event_loop.no`: gyldig TLS, feil vertsnamn, mTLS og avvising
  av ustøtta CRL/OCSP-policy.
- `test_krypto_token_security.no`: format, lengder og ugyldige argument.
- `test_linux_x86_64_aot_rng_failclosed.no`: ekte seccomp-avslag på getrandom,
  inkludert offentlege krypto-API gjennom ein nybygd native-binær.

`builtin.network_operation` er bakt inn i seed/runtime. Endra `.no`-kjelde
åleine oppdaterer ikkje ein gammal installert binær. Bygg og test ein ny
runtime før promotering. `tests/fixtures/tls_trust_public_boundary.no`
kontrollerer avvising ved den offentlege transportgrensa;
`tests/fixtures/tls_trust_public_loopback.no` fullfører eit verifisert handtrykk,
utvekslar data og avviser feil vertsnamn. Køyr dei med den nye runtime-en eller
som nybygde native-binærar.
