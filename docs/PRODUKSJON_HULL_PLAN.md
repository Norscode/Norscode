# Produksjonsplan — alt som manglar før hosting-produktet (#4/#5/#6)

**Dato:** 2026-08-16
**Grunnlag:** verifisert mot faktisk kjelde (ikkje berre STATUS.md). Sjå kjeldereferansar per punkt.

## Utgangspunkt: kva som IKKJE er problemet

Sjølvstendigheita/språket er i praksis i mål: `selvstendighet L1–L6` BESTÅTT, Gen1/Gen2 byte-paritet, `runtime_status` 19/22 stabile, `stdlib_status` 57/61 stabile. **Du byggjer produkt oppå ei sjølvstendig kjerne — ikkje ferdig språket.**

Tre ting i den opphavlege hòl-tabellen var utdaterte og er korrigerte her:
- **`std.mail_server` er alt STABIL** (`std/stdlib_status.no:38`, `std/mail_server.no:530`) — ikkje eksperimentell.
- **`./bin/nc serve` er rein Norscode** via `selfhost/serve_runner.no` med native socket — ingen Python (`tools/nc_serve.py` finst ikkje).
- **Multi-site finst alt** i `std/https_front.no` (SNI/Host-basert per-domene-ruting, 421 fail-closed).
- Den gamle «løpsk rekursjon på djupe HTML-sider»-blokkeringa er **løyst** (djupgrense 256→4096, opptil 16384; `docs/STATUS.md:180`).

## Skiljet som styrer planen: kode vs. attestasjon

| Kategori | Kva det betyr | Rekkefølge-konsekvens |
|---|---|---|
| **Manglar kode** | Reell ny implementasjon trengst | Kan gjerast no, lokalt |
| **Manglar attestasjon** | Koden finst; ventar ekstern verifisering/revisjon/plattformkøyring | Parallelt spor, dels avhengig av eksterne partar |

---

## Arbeidsstrøymar (M1–M7)

### M1 — Små primitiver ✅ FERDIG *(2026-08-16)*
Billegaste opplåsingar. Ingen plattformattestasjon.

| Oppgåve | Resultat |
|---|---|
| **pure SHA-1** | ✅ `std/sha1.no` (rein pure-VM, `hash`/`hash_bytes`/`hash_raw`/`hash_raw_tekst`). Test `tests/test_sha1_rfc3174.no` — RFC 3174-vektorar + RFC 6455-handshake-vektor. |
| **WebSocket-handshake** | ✅ `std/ws.no` `_ws_aksept_nøkkel` koplar no til pure `sha1` + `base64` (var ikkje-funksjonelt før). Test `tests/test_ws_handshake_rfc6455.no`. |
| **base64 encode** | ✅ Fanst alt pure (`std/base64.no`), stadfesta. |
| **bytes→text binær** | ✅ Fanst native (`vm.no:4466`), stadfesta. |
| **char_from_code** | ✅ `std/slug.no` byter til `builtin.chr` (ekte native primitiv). Fann + fiksa dessutan pre-eksisterande `tekst_saman`→`builtin.join` (8 stader) som gjorde `url_decode` ikkje-køyrbar standalone. |
| **PBKDF2 32-byte-tak** | ✅ Heva til 1024 byte i `std/krypto.no`; native rask-veg berre ≤32, pure fleirblokk for lengre. Test `tests/test_pbkdf2_multiblock.no`. |

**Resultat:** WS-handshake funksjonell + PBKDF2 klar for nøkkelutleiing. Alle testar grøne, ingen regresjonar.

**Gjenstår i M1-sfæren (låg prioritet):** `_til_json` i `std/ws.no:266` har same latente `tekst_saman`-kall — ryddast naturleg i M2 når ws byggjast ut.

---

### M2 — WebSocket ✅ FERDIG *(2026-08-16)*
`std/ws.no` var ein **stub**: heile transportlaget kalla ikkje-eksisterande `builtin.ws_*`. Valde veg A (rein Norscode over eksisterande socket-ABI).

| Oppgåve | Resultat |
|---|---|
| **Frame-codec verifisert** | `bygg_frame`/`parse_frame` låst mot RFC 6455 §5.7-vektorar. Test `tests/test_ws_frame_rfc6455.no`. |
| **Binær-sikker payload** | `frame_til_tekst` fiksa (slepte før alt utanfor ASCII 32-126) → `builtin.chr` per byte; ny `frame_til_bytes`. |
| **Ekte socket-transport** | `send`/`send_binar`/`motta`/`motta_med_timeout`/`lukk` skriv/les ekte frames over `std.socket` hex-I/O (`native_skriv_hex`/`native_les_hex`), byte-eksakt frame-lesing (`_les_full_frame`). `fest_socket(conn, sock)` bind ei tilkopling til ein socket. Gammal `builtin.ws_*` behalden som valfri fallback. |
| **Ende-til-ende live-test** | `tests/test_ws_transport_loopback.no` — server↔klient over ekte native loopback-socket (tekst, newline, UTF-8, pen lukking). Registrert i `tools/nc_test.no` med net.tcp/127.0.0.1-scope. |

**Resultat:** WebSocket verkar reelt over native socket. Server→klient umaskert (RFC-korrekt hosting-veg). Alle testar grøne via `./bin/nc test`.

**Gjenstår (framtidig, ikkje-blokkerande):** server-loop `serve` + gruppe/broadcast (`ws_group_*`) treng eit tilkoplingsregister + event-loop; klient-maskering brukar lengd-derivert nøkkel (funksjonell, men ikkje-tilfeldig — berre relevant for klient-modus, ikkje hosting).

---

### M3 — Multi-site HTML+HTTPS produksjonsbevis ✅ FERDIG *(2026-08-16)*
Byggjeklossane fanst; det som mangla var **éin ende-til-ende live-gate**.

| Oppgåve | Resultat |
|---|---|
| **Live multi-site serving** | `tests/test_https_front_live_multisite.no` — ekte reverse-proxy over native socket: **to backend-tenarar** (kvar sin native tråd), Host/SNI-basert vhost-ruting til rett backend, ingen krysslekkasje. Registrert i `tools/nc_test.no` (net.tcp/thread.spawn/127.0.0.1). Grøn i suiten. |
| **Djup HTML** | 600-nivå nøsta HTML passerer intakt gjennom proxyen — refuterer den gamle «løpsk rekursjon på djupe sider»-uroa i gap A. |
| **Fail-closed** | 421 ved SNI/Host-mismatch, 502 ved daud backend, ingen backend-adresse lekkjer i feilrespons. |
| **TLS-terminering** | Verifisert av eksisterande `tests/test_native_tls_event_loop.no` (passerer lokalt) + `test_tls_http.no`. `https_front.opne` brukar identisk `socket.native_tls_lytt(cert,key)` som `tls_http`. Merk: TLS-backend finst i `norscode_native` (testløpar/CI), men **ikkje** i `./bin/nc run`-wrapperen. |

**Dette lukkar gap A / #5.**

**Gjenstår (valfri CI-herding):** éin samla gate som køyrer `serve_once`-over-TLS med live backend (3-aktør: TLS-klient + front + backend) i staden for å bevise TLS og proxy separat.

---

### M4 — NorsDB FROM-subquery ✅ FERDIG *(2026-08-16)*
Oppdaging: den **aktive** databasen er `std.db` → `std.norsdb` → SQL-motor i `selfhost/vm.no` (`_vm_db_parse_select`). Den korrekte fix låg i vm-kjernen (byte-paritet-gate). Etter avtale valde vi **wrapper-tilnærminga** (låg risiko).

| Oppgåve | Resultat |
|---|---|
| **FROM-subquery i std-laget** | `std/norsdb.no`: `query_text`/`query_int` detekterer `FROM ( <subquery> )`, køyrer indre spørjing strukturert (`vm._vm_db_select_rows_with_meta`), **projiserer** til inner-kolonnar, materialiserer som midlertidig tabell i db-objektet, skriv om ytre, køyrer, ryddar temp. **Rører ikkje `selfhost/vm.no`.** |
| **Dekning** | Filter på subquery-kolonne, COUNT, **nøsta** subquery (rekursivt), ORDER/LIMIT på ytre, alias (`AS b`), projeksjons-avgrensing (skjulte kolonnar), normal spørjing uendra. Test `tests/test_norsdb_subquery.no`. |
| **Verifisert** | Alle tilfelle grøne via `./bin/nc run`. |

**Miljømerknad:** db-testar via `./bin/nc test` feilar *lokalt* med `Ukjent variabel: vm_active_functions` — ein **pre-eksisterande** stale precompilert vm-NCB (rammar òg pristine `test_norsdb_default`), ikkje M4. Verifisert i staden via `./bin/nc run` (kompilerer vm frå kjelde, ~5–6 min kald). Testen køyrer i CI med gyldig NCB.

---

### M5 — Attestasjon: nettverk / DNS / ACME / sikkerheit *(ekstern-avhengig, parallelt)*
Desse fire er **kode-komplette** men merkte eksperimentell/delvis. Det som står att er **verifisering, ikkje implementasjon** (unnatak: rein Norscode TLS, sjå M7).

| Modul | Kva står att | Type |
|---|---|---|
| `network_runtime` | Ekte Windows SChannel TLS 1.3-handshake for gjeldande kandidatgenerasjon (Wine kan ikkje TLS 1.3 → treng ekte Windows) | Attestasjon |
| `std.dns` | Ekstern multi-plattform interop + release-/providergate (svarar alt ekte Pebble dns-01 i CI) | Verifisering |
| `std.tls_acme` | Ekstern sikkerheitsrevisjon (full utstedelse alt verifisert mot Pebble) | Revisjon |
| `std.sikkerheit` | Ekstern sikkerheitsrevisjon + Windows AppContainer/SChannel-attestasjon | Revisjon |
| `std.domenehost` | Ingen eigen kode — blir stabil automatisk når dei tre over er grøne | Avleidd |

**Handling:** (1) køyr `tools/windows_runtime_attestation.no` mot ny kandidat → løftar `network_runtime`. (2) Planlegg ekstern tredjeparts-revisjon av krypto/TLS/ACME. (3) DNS-interop-testmatrise mot fleire eksterne resolverar.

**`std.mail_server` treng ingenting her — den er alt stabil.**

---

### M6 — Fjern-eksekvering + provisjonering 🟡 PÅBYRJA *(2026-08-16)*
**Den einaste posten som er reelt ny arkitektur på kritisk sti.** Kartlegginga stadfesta: ingen SSH-klient, ingen fjern-agent, ingen remote-exec i repoet.

**Arkitektur vald: (b) minimal agent-protokoll over verifisert native socket/TLS** (ikkje rein Norscode SSH-klient). Grunngjeving: gjenbrukar heile den verifiserte stacken (M2/M3-socket, `std.prosess` `norscode-native-process-v1`, `std.sha256` HMAC), langt meir sjølvstendig enn tusenvis av linjer SSH-protokoll.

| Delsteg | Status |
|---|---|
| **1a. Protokoll-logikk** | ✅ `std/fjern_agent.no` bygd + kompilerer: HMAC challenge-response (fersk nonce = replay-vern, konstanttid), lengde-ramma protokoll, allowlista shell-fri exec via `std.prosess`. **Logikken verifisert** av `tests/test_fjern_agent_signer.no` (determinisme, token/nonce/argv-skilje, konstant_lik, 64-hex SHA-256) — i pure-suiten. |
| **1b. Live socket-transport** | 🔴 **Blokkert av runtime-socket-åtferd** i stage0. Loopback-testen (`tests/test_fjern_agent_loopback.no`) er skriven men IKKJE i pure-suiten. Sjå funn under. |
| **2. Fjern-provisjonering** | ⬜ push generert config (`deploy.no`) + køyr installatørar + `systemctl` remote → «ett-klikks bar-server-oppsett». |

**Lokal prosess-spawning er alt moden** (`norscode-native-process-v1`). **Dette lukkar gap C / #4.**

### Runtime-funn (blokkerer live loopback, må fiksast i socket-laget/stage0)
Djup feilsøking (agent+klient over loopback i to trådar) avdekte fleire inkonsistensar i dei native socket-primitivane under dette mønsteret:
- **Asymmetrisk tekst/hex-støtte per side:** `native_les` (tekst) vs `native_les_hex` gir ulikt resultat på connect- vs accept-sida.
- **Kryss-tråd leverings-svelting:** to trådar som les samtidig svelt ut leveringa; utan yield kom nonce først etter deadline. Ingen `builtin.sov`/`sleep`/`thread_yield` finst i stage0 (berre `native_poll_vent`, som ikkje var nok her).
- **Modul- vs `__main__`-avvik:** identisk lese-loop verkar inline (`__main__`) men ikkje frå ein std-modul-funksjon (module-klienten fekk mangla/feil data).
- `builtin.tid_ms` verkar i modul (utelukka som årsak).

**Tilrådd veg vidare (val for brukaren):**
1. **Ein-rundtur-redesign** (som `std.https_front`, det einaste beviste mønsteret): klienten sender FØRST `{argv, ts, hmac(token, ts|argv)}`, agenten svarar ÉIN gong og lukkar. Tidsstempel-nonce (± vindauge + valfri nonce-cache) gir replay-vern utan server-først-challenge. Unngår den bidireksjonelle ping-pongen som trigga quirks.
2. **To-prosess / TLS-deploy:** køyr agent og klient som separate `nc run`-prosessar (ekte fjern-oppsett) i staden for trådar.
3. **Fiks socket-laget** (tekst/hex-konsistens + ein yield-primitiv) og køyr den eksisterande loopback-testen.

---

### M7 — Rein Norscode TLS/krypto *(kode, langsiktig — lågast hast)*
ACME/nettverk/sikkerheit brukar framleis **OpenSSL-overgangsadapter** for delar av kryptoen. Ein rein Norscode TLS 1.3-stakk finst delvis (`std/tls13_handshake.no`, `tls13_keyschedule.no`, X25519/Ed25519/ChaCha20-Poly1305).

**Handling:** fullfør rein TLS 1.3 og erstatt OpenSSL-adapteren. **Trengst for full sjølvstende-*reinheit*, ikkje for funksjon** — OpenSSL-vegen verkar i dag. Gjer sist.

---

### E — Toolchain *(kontinuerleg, blokkerer ingenting)*
NCB treg kald-start, multi-modul-import-quirks. Løpande forbetring; ikkje på kritisk sti.

---

## Kritisk sti og tilrådd rekkefølge

```
M1 (dagar) ──► M2 (WebSocket)
   │
   └──► M3 (multi-site HTML+HTTPS bevis)  ◄── lukkar #5
   
M4 (NorsDB subquery)   — uavhengig, medium prioritet
M5 (attestasjon)       — parallelt, ekstern-avhengig  ◄── lukkar #6-modning
M6 (fjern-eksekvering) — START TIDLEG (lengst) ◄── lukkar #4
M7 (rein TLS)          — sist, reinheit ikkje funksjon
```

**Anbefaling:**
1. Gjer **M1** no (dagar) — billeg, låser opp fleire ting.
2. Start **M6** parallelt tidleg — det er den lengste stanga og einaste ekte nye arkitekturen.
3. **M3** gir raskt eit demonstrerbart produkt (#5 multi-site hosting).
4. **M5** køyrer som eige attestasjonsspor mot eksterne partar.
5. **M2/M4** etter behov; **M7** til slutt.

## Kjeldereferansar
- Status: `docs/STATUS.md` (132–140, 180, 184, 188–193, 221–235), `std/stdlib_status.no:27,35–39`, `std/runtime_status.no:29`
- Serve/TLS/multi-site: `selfhost/serve_runner.no`, `std/https_front.no`, `std/socket.no:246`, `std/tls_http.no`
- Primitiver: `std/base64.no`, `std/ws.no`, `std/krypto.no:145`, `std/sha256.no`
- Prosess/fjern: `std/prosess.no`, `std/runtime/process_abi.no`, `std/deploy.no`, `std/linux_drift.no` (alle lokal-host)
- NorsDB: `NorsDB/norsdb_crud.nors:62`
```
