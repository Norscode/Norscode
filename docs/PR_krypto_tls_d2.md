# D2: rein Norscode TLS 1.3 + web-PKI (X.509 med Ed25519/RSA/ECDSA)

Grein: `krypto-tls-primitiver` → `main`
Siste commit: `5d4ca84`

## Samandrag

Denne PR-en fører D2-kryptokjernen i rein Norscode: eit fullt TLS 1.3-lag
(record + handshake + 1-RTT-handtrykk) og web-PKI X.509-kjedevalidering med dei
tre vanlege signaturalgoritmane. Ingen OpenSSL/C i kjeda — alt er rein Norscode,
verifisert mot RFC-vektorar og ekte openssl-genererte sertifikat. Milepæl A er
stengt (generasjon G-A dokumentert) og Milepæl B er diagnostisert med runbok.

## Nye modular (`std/`)

- `tls13_record.no` — TLS 1.3 record-lag (RFC 8446 §5) over ChaCha20-Poly1305.
- `tls13_handshake.no` — handshake-meldingskodek (RFC 8446 §4), bounds-herda parserar.
- `tls13_handshake_flow.no` — full 1-RTT klient↔tenar-handtrykk (loopback).
- `bigint.no` — unsigned bigint (base 2^16, Knuth-D divmod + modexp).
- `rsa.no` — RSA PKCS#1 v1.5 SHA-256 verifikasjon (RFC 8017 §8.2.2).
- `ecdsa_p256.no` — ECDSA-verifikasjon på NIST P-256 (Jacobian-koordinatar).
- `x509_chain.no` — X.509-kjedevalidering: gyldigheit, SAN/hostname (RFC 6125),
  signaturkjede (Ed25519 + RSA-SHA256 + ECDSA-P256) og trust-anchor.

## Testdekning (16 grøne steg i Krypto-smoke, CI run 31245505530)

RFC-vektorar: SHA-512 (NIST), HKDF (RFC 5869), X25519 (RFC 7748), Ed25519
(RFC 8032), TLS 1.3 nøkkelplan + record + handshake-kodek + differensial-KAT
(heile RFC 8448-løyndomstreet). Fail-closed: parser-fuzzing. web-PKI: bigint mot
Python-vektorar (inkl. 2048-bit modexp), RSA mot ekte 2048-bit openssl-nøkkel,
ECDSA mot ekte openssl-signatur, og X.509-kjede for Ed25519/RSA/ECDSA mot ekte
openssl-genererte root+leaf-kjeder (gyldig kjede + fail-closed på feil
vertsnamn/tukla signatur).

## Dokumentasjon

- `docs/GENERATION_G-A_MANIFEST.md` — A7 frysemanifest (G-A på `main@52593ad9`,
  alle stage0-hashar mot SHA256SUMS, A2-attestasjon registrert).
- `docs/MILEPÆL_A_RUNBOK.md` — oppdatert med payload-drift-diagnose + stage0-gap.
- `docs/MILEPÆL_B_RUNBOK.md` — B0–B4 host/CI-steg + empirisk sandkasse-diagnose.
- `docs/SELVSTENDIGHET_SLUTTPLAN.md` — D2/A/B-status oppdatert.
- `docs/attestations/…sigstore.json` — arkivert GitHub-signert Windows-attestasjon.
- `.github/workflows/krypto-smoke.yml` — utvida til å gate heile D2 + web-PKI.

## Kva denne PR-en IKKJE gjer (ærleg restanseliste)

- **Ikkje** socket-integrasjon av handtrykket (loopback i minnet førebels).
- **Ikkje** live differensial-interop mot OpenSSL-adapteren (krev adapteren/host).
- **Ikkje** konstanttidsrevisjon eller ekstern sikkerheitsrevisjon (D3).
- **Ikkje** CRL/OCSP-revokeringssjekk.
- **Ytingsmerknad:** RSA/ECDSA-verifikasjon er tung på den fortolka stage0
  (~12–50 s/verifikasjon) og bør køyrast på det native tillitsankeret i produksjon.

## Generasjonsmerknad (viktig for reviewers)

D2 er **ikkje** del av G-A (`main@52593ad9`). Ved merge inn i `main` lagar dette
ein ny generasjon; Milepæl A-portane (særleg A2 Windows-attestasjon + full strict)
må køyrast på nytt på merge-commiten før den nye generasjonen kan frysast. Sjå
`docs/GENERATION_G-A_MANIFEST.md`.
