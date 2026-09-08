# TLS / trådar / sandbox — status og hòl-kartlegging

Revisjon 2026-09-08. Sidespor: gjere TLS / trådar / sandbox-profilar så komplett og
verifisert som råd i **rein Norscode**, uavhengig av seed-promoteringa.

Testar køyrast med capabilities
`env.read,env.write,process.exec,thread.spawn,net.tcp,net.dns,disk.read,disk.write,jit.execute`
og disk-root `$PWD,.,/tmp,/private/tmp` via
`NORSCODE_VM_CAPABILITIES` / `NORSCODE_VM_DISK_ROOT` og `./bin/nc run`.

Sentralt skilje i denne kartlegginga:

- **Rein Norscode (VM-berekna)** — heile logikken er `.no`-kode; køyrer likt under
  `nc run`-VM-en. Dette er sporet vi eig her.
- **Native-backend** — modulen er ei tynn innpakking rundt native runtime-builtins
  (`builtin.thread_pool`, `builtin.thread_spawn`, `builtin.process_spawn_argv`,
  `builtin.ssl_*`, `socket.native_tls_*`). Desse builtinane er bakt inn i `bin/nc`
  (native_execution) og kan **ikkje** endrast frå std-nivå. Under `nc run`-VM-en
  degraderer nokre til feilstatus eller heng. Dette høyrer til seed-/runtime-sporet.

---

## 1. TLS 1.3 (rein Norscode kryptokjerne)

Den reine TLS 1.3-stakken (PR #181) er komplett for den obligatoriske suiten
`TLS_AES_128_GCM_SHA256` + `TLS_CHACHA20_POLY1305_SHA256`, X25519 og Ed25519, og er
verifisert steg-for-steg mot RFC 8448 og NIST/FIPS-vektorar.

| Modul | Kva er dekt | Kva manglar / avgrensing | Testdekning |
|-------|-------------|--------------------------|-------------|
| `std/aes.no` | AES-128 blokkchiffer (FIPS-197 key-schedule + rundar), encrypt-block | Ingen AES-256 blokk (trengst for `TLS_AES_256_GCM_SHA384`, valfri suite) | `test_aes128_fips197` (FIPS-197 App. B + C.1) |
| `std/aes_gcm.no` | AES-128-GCM AEAD (seal/open), 96-bit IV, GHASH, konstant-tid tag-lik | Berre 128-bit nøkkel | `test_aes_gcm_nist` (NIST GCM TC2/TC3) |
| `std/tls13_keyschedule.no` | HKDF-Expand-Label, Derive-Secret, early/handshake/master secret, transcript-hash (SHA-256) | Berre SHA-256 (ingen SHA-384-plan) | `test_tls13_expand_label`, `test_tls13_keyschedule_rfc8448`, `test_tls13_differential_kat` (heile RFC 8448 løyndomstre) |
| `std/tls13_record.no` | Record-lag §5: AES-128-GCM **og** ChaCha20-Poly1305, per-post nonce (§5.3), inner-plaintext + content-type, fail-closed dekryptering | Ingen record-padding (kosmetisk), ingen key-update | `test_tls13_record_aesgcm`, `test_tls13_record_chacha`, **`test_tls13_traffic_keys_rfc8448` (NY)** |
| `std/tls13_handshake.no` | Kodek §4: ClientHello/ServerHello/EncryptedExtensions/Certificate/CertificateVerify/Finished, ext-parsing, fail-closed lesarar | Ingen HelloRetryRequest, ingen PSK/0-RTT-ext, éin suite/gruppe i byggarane | `test_tls13_handshake_codec`, `test_tls13_fuzz_parsers` (parser-robustheit) |
| `std/tls13_handshake_flow.no` | Full 1-RTT tilstandsmaskin i minnet (loopback): ECDHE X25519, nøkkelplan, CertificateVerify Ed25519, Finished-MAC, app-data round-trip, tuklings-avvising | Ingen re-handshake, ingen HRR, loopback (ikkje socket) | `test_tls13_handshake_flow` |
| `std/tls_acme.no` | ACME-protokoll (RFC 8555) contract-nivå | (klientflyt; native HTTP utanfor) | `test_tls_acme_contract` |
| `std/tls_http.no` | HTTPS-tenar-konfig, HSTS, mTLS/CRL/OCSP-oppsett, cert-validering (metadata) | Transport via `socket.native_tls_*` (OpenSSL-backend, ikkje rein Norscode) | `test_tls_http` |
| `std/ssl.no` | Python-`ssl`-kompat-shim | Heilt bygd på `builtin.ssl_*` (native OpenSSL) — ikkje rein Norscode | (ingen eigen; native) |
| `std/https_front.no` | HTTPS-frontkonfig | Bygd på native transport | (via tls_http) |

**Hòl tetta i denne revisjonen:** `test_tls13_differential_kat` verifiserte record-lagets
IV mot RFC 8448, men den **suite-avhengige 16-byte AEAD-nøkkelen** (write_key) vart aldri
sjekka mot RFC-fasit. Ny `test_tls13_traffic_keys_rfc8448` verifiserer at
`rec.traffic_keys_aesgcm(secret)` gjev nett RFC 8448-key **og** -iv for handshake- og
applikasjonstrafikk, klient og tenar (8 KAT-verdiar), pluss ein ende-til-ende
krypter/dekrypter-round-trip med dei ekte RFC 8448-tenarnøklane.

**Står att (utanfor mandatory suite, valfritt):** `TLS_AES_256_GCM_SHA384` (krev AES-256
blokk + SHA-384 nøkkelplan), HelloRetryRequest, PSK-resumption/0-RTT, key-update. Ingen av
desse er obligatoriske for RFC 8446-samsvar på 1-RTT-nivå.

---

## 2. Trådar

| Modul | Kva er dekt | Kva manglar / avgrensing | Testdekning |
|-------|-------------|--------------------------|-------------|
| `std/tråd.no` (kooperativ manager) | Rein Norscode: `ny_manager`, `ny_tråd(_med_args)`, `start`/`steg_alle`/`join(_alle)`/`join_med_grense`, mutex-simulering (depth/eigar/blokkering), atomiske (CAS/fetch_add/load), delt tilstand, events, barrierar, deadlock-deteksjon | Kooperativ (steg-basert), ikkje ekte parallellisme | `test_thread_sync_lifecycle` (rein), `test_native_thread_abi` (ABI-kontrakt, rein data) |
| `std/tråd.no` (native pool/sync) | Innpakking av `builtin.thread_pool` / `builtin.thread_spawn` / `builtin.thread_sync` | **Native-backend** — krev pthread-runtime i `bin/nc`; degraderer/heng under `nc run`-VM | `test_native_thread_pool`, `test_native_thread_sync_backend`, `test_thread_native_backend_lifecycle` (alle native) |
| `std/runtime/thread_abi.no` | ABI-request/-resultat-bygging og validering (rein data) | — | `test_native_thread_abi` (grøn), `test_native_thread_backend` (grøn) |
| `std/sched.no` | Rein Norscode event-loop: `enter`/`enterabs`/`call_soon`, prioritetskø, `run_ferdig`, futures (ny/fullfør/avvis/kanseller/vent), IO-poller-registrering, gjentakande timerar | IO-poll bruker `builtin.network_operation` (native) for ekte sockets | `test_sched_event_loop`, `test_sched_futures`, `test_sched_future_success`, `test_sched_io_poller`, `test_sched_vm_await` |
| `std/multiprocessing.no` | Prosess-kontrakt (rein) + native argv-spawn | Native-delen via `builtin.process_*` | `test_multiprocessing_contract_text` (rein, grøn), `test_multiprocessing_native` (grøn) |

**Vurdering:** den reine tråd-flata (kooperativ manager + sched-event-loop/futures) er
komplett og deterministisk verifisert. Den **native pthread-backenden** er den einaste
raude/hengande delen, og han ligg i runtime-builtinane (native_execution) — utanfor det
som kan rørast frå std-nivå i rein Norscode.

---

## 3. Sandbox-profilar

| Modul | Kva er dekt | Kva manglar / avgrensing | Testdekning |
|-------|-------------|--------------------------|-------------|
| `std/runtime/process_abi.no` | Request-bygging med sandbox-felt (`sett_sandbox`: profil, minne, fd-grense, inherit_env), async-spawn/-wait/-operation-request (rein data) | — | (via `test_process_os_sandbox`) |
| Sandbox-håndheving | Profilar `no-network` / `no-write` / `restricted` via `builtin.process_spawn_argv` / `builtin.process_operation` | **Native-backend** — seatbelt (macOS) / tilsvarande; håndhevinga er i runtime, ikkje rein Norscode | `test_process_os_sandbox` (native; krev seatbelt-backend) |
| VM-capabilities/policy | `selfhost/vm.no` (capability-gating: env/disk/net/thread/process/jit) | **Ikkje redigerbar** (fragmentmodul, seed-spor) | (dekt av at alle testar køyrer under capability-gate) |

**Vurdering:** sandbox-*policyen* (kva profil, kva grenser) byggjast som rein Norscode-data
i `process_abi`, men sjølve *håndhevinga* av profilane skjer i native runtime (seatbelt).
Det finst inga rein-Norscode sandbox-håndheving å tette på std-nivå; VM-capability-gaten i
`vm.no` er utanfor mandat.

---

## Oppsummert honnør-vurdering

- **TLS 1.3 rein kryptokjerne:** i praksis ferdig for mandatory-suiten, RFC 8448/NIST-
  verifisert i kvar avleiing. Ny KAT lukka det siste umerka gapet (AEAD-nøkkel vs RFC).
  Att står berre valfrie suiter/utvidingar (AES-256/SHA-384, HRR, PSK/0-RTT).
- **Trådar:** rein kooperativ + async-flate ferdig og grøn. Native pthread-backend
  (pool/sync) er runtime-avhengig og ligg utanfor rein-Norscode-mandatet.
- **Sandbox:** policy-bygging rein og dekt; håndheving er native (seatbelt), utanfor mandat.
