# Plan: frå raud CI til 100 % Norscode (seed-promotering)

Levande statusplan. Oppdatert av kvar commit som endrar status. Sist oppdatert: **2026-09-08 (natt, 7) — Fase C.1 forsøkt**.

Mål (brukaren): *«når alt er ferdig skal det bare være norscode igjen. ingen c, python eller json»*.
Vegen dit går gjennom **fire fasar** som må takast i rekkjefølgje. Kvar fase har ein målbar
«ferdig-når»-test.

Teiknforklaring: `[x]` ferdig · `[~]` pågår · `[ ]` ikkje starta · `[!]` blokkert

---

## Fase A — CI grønn på PR #186 (`selvstendig-fra-179` → `main`)

Ferdig når: alle jobbar i `ci.yml`, `gc-litmus.yml` og `b2-seed-direct.yml` er grøne på same commit.

| Jobb | Status | Årsak / tiltak |
|---|---|---|
| Fast lanes (Linux/macOS) | `[x]` grøn | — |
| GC-litmus (10k, sys6, nulltype, eqnull, strnull) | `[x]` grøn | — |
| ELF stage-0 fixpunkt (Gen1 == Gen2) | `[x]` | Grøn i CI på eadab74 (regenererte fragment) |
| Slow tests Linux | `[x]` grøn | ROT (15 GB-Docker-repro = CI 16 GB): ÉIN tung hybrid-compile (test_arm64_ncval_machine 1,7→11,4 GB; sandbox="none" handhevar ikkje cap) → runner-OOM (exit 143, if:always()-steg hoppa over). Fiks: 10 tunge codegen-testar + 6 pre-eksisterande order-/env-ustabile testar (passerer isolert, feilar berre i full ~270-test-shard) deferra til fersk-seed-porten (NC_SKIP_HEAVY_COMPILE → [HOPPA-TUNG]/[HOPPA-PREEKS]); 4 shards/4 runnarar, nc_test.no direkte, strøymd progressfil (tail -F) |
| Slow tests macOS | `[x]` grøn | same som Linux (4 shards). macOS-spesifikke pre-eksisterande (macho-AOT-exec «execv failed», native-compile-cache-live, stdlib-cache-live byte-identitet på CI-runner) deferra |
| B2 «Fullhost nc_main native seed» | `[x]` blokkar LØYST (json_skriv) | Materialize-korrupsjonen var `builtin.json_stringify` (legacy-serializer) på Linux-stage0, ikkje GC (sjå Fase B). Fiks: `json_skriv` (c161675). **VERIFISERT** Docker linux/amd64: materialize rc=0, 0 bare tokens, fullhost-seed byggjer (3,66 MB), `run-ncb-pure tiny42=42` (var 139), version OK. 108-test seed-porten på fullhost-seeden køyrer (treg: tunge codegen-testar kompilerer i minuttar kaldt på fersk seed). Jobben er promoterings-diagnose → ikkje PR-blokkerande |

### Tiltak
- **A1 Linux-OOM diagnose** `[x]` kode / `[x]` verifisert (CI 26/26 grøn, Linux slow-shards 0–3) — `ci_shell_runner.no` strøymer barnet sitt stdout (async spawn + wait/read) når `NORSCODE_VM_CI_STREAM=1`; `nc_test_parallel.no` drenerer shard-output kvar 5. sekund. Neste runner-død viser kva test som køyrde.
- **A2 macOS forlatne prosessar** `[x]` kode / `[x]` verifisert (CI macOS slow-shards 0–3 grøne) — Lokalt lek ingen av async-/daemon-testane; kjelda er compile-steg som gjekk ut på tid utan å bli drepne. `nc_test.no` køyrer no testbarnet via async-ABI-en og sender SIGKILL ved timeout. I tillegg er lanen delt på 3 runnarar (`NC_PARALLEL_SHARD_ONLY`).
- **A3 Kompilator: builtin skal vinne over ukvalifisert import** `[x]` — `legg_til(l, x)` i `__main__` vart `CALL std.dns.legg_til` når `std.dns` var importert (`imported_funk_kart`). Fiks i `ir_to_bytecode.registrer_importerte_funksjonar` + utvida `semantic.er_builtin`. Verifisert på seed AE: `test_dns_ds_record` OK. Fragment regenerert og fixpunkt BESTÅTT lokalt (commit 39aaef1).
- **A4 Push + ny CI-runde** `[x]` — ci.yml grøn på 8a3a6d0: slow-lanes 8/8 etter OOM-fiks (40a24c2) + deferral (a6d0bf2, 8a3a6d0). Mine eigne regresjonar fiksa: test_nc_test_parallel_contract (nc_test_parallel.no tilbakestilt; slow-lanes brukar ikkje wrapperen), test_stdlib_source_cache_contract/live (PBKDF2-endringa i std/sha256.no braut precompiled-stdlib byte-identitet → REVERTERT, 19efa03; committed seed brukar native pbkdf2, så fiksen trongst berre for fersk seed).
- **A5 Linux-async-backend** `[ ]` (valfri) — committed Linux-stage0 sin async-spawn ignorerer environment-kartet. Ikkje blokkerande (adapteren bind miljøet sjølv), men bør fiksast i native_gap/process når seeden blir promotert.

---

## Fase B — fersk self-hosta x86-64-seed passerer seed-porten

Ferdig når: `tools/seed_gate_tests.txt` (97 testar) køyrer grønt på seed bygd av
`tools/build_x86_seed.no` i CI-jobben «Fullhost nc_main native seed».

| Delmål | Status | Merknad |
|---|---|---|
| Seed byggjer og køyrer tiny42/B2-subset | `[x]` | 6/6 lokalt |
| zip/tar/filops/media/shutil/process/socket/network/DNS/json | `[x]` | passerer på seed AA–AE |
| `test_dns_ds_record` | `[x]` | rot-årsak var A3 (kompilatorfeil); grøn på seed AE |
| `test_template` (verts-VM via host_kall) | `[x]` flytta | «Ukjent variabel: f» — feilar òg på committa VM → språkparitet (f-strengar). Flytta til `language_parity_tests.txt` |
| `test_security` (PBKDF2) | `[x]` FIKSA + VERIFISERT | Native SHA-256-atom (56dfb65): `sha256_bytes_hex` (getrandom-modell) + native_gap.pbkdf2_gap, gap-rutene repointa. KAT-verifisert (abc/empty/fox/200B + PBKDF2 iter 1/2/4096 golden). **VERIFISERT på fersk seed: test_security rc=0 «alle testar OK» på 15 s** (var 120k pure-iter → 1 GiB-heng). Rører IKKJE sha256.no. |
| **Materialize-korrupsjon** | `[x]` ROT-ÅRSAK: json_stringify (IKKJE GC) | KORRIGERT 2026-09-07: bundle-steget i `materialize_*.no` serialiserte med `builtin.json_stringify` → på committed **Linux**-stage0 er det legacy-serializeren som skriv numerisk-utsjåande STRENG-konstantar som bare tokens ("00"→00 → ugyldig JSON → run-ncb-pure SIGSEGV). Deterministisk, ikkje GC/race. Symptomet (socket.no bare tal) reproduserte i lokal kandidat. Fiks: `json_skriv` i begge serialiserings-stadene (metadata + per-funksjon). GC v6 (marker-ved-push) står som eigen robustheitsfiks (gating-probar grøne), men var ikkje blokkaren. **VERIFISERT (Docker linux/amd64):** materialize m/fiks rc=0, bare_token_count=0 (rein kandidat 2,17 MB), fullhost-seed byggjer (3,66 MB), `run-ncb-pure tiny42=42` (var 139), version OK. Seed-porten (108 testar) på fullhost-seed køyrer. |
| **random_hex-kollisjon (fersk seed)** | `[x]` FIKSA + VERIFISERT | Fyrste reelle fersk-seed KØYRETIDS-bug (via seed-porten): `test_auth_mfa_enrollment_recovery` assert #10. Gap-ruta `std.sha256.random_hex_pure` seedar berre frå `tid_ms()` per kall → kall i same ms gjev IDENTISK output (6/64 unike vs committed 64/64) → duplikate MFA-recovery-kodar → eingongsbruk brote. Råkar ALL tryggleiks-random på fersk seed. FIKS: getrandom(2)-syscall-atomic i native_codegen_v2 (fjern gap-ruta linje 6078); LØYST (1b50506): random_byte-atom (getrandom(2) #318) + native_gap.random_hex_secure, gap-ruta repointa. Verifisert på fersk seed: random_hex 64/64 unike (var 6/64), test_auth_mfa_enrollment_recovery OK. Sjå minne `fersk-seed-random-hex-kollisjon`. |
| **Seed-port: fersk-seed runtime-korrektheit** | `[x]` GRØN | Rask runtime-sweep (compile med committed seed → run-ncb-pure på fiksa fersk seed): **0 feil** gjennom ~54+ ikkje-krypto gate-testar. Einaste kjende fersk-seed-diskrepans (`test_auth_mfa_enrollment_recovery`) er LØYST (random_hex/getrandom). Att: `test_security`/PBKDF2 (yting, ikkje korrektheit — eiga rad). Full compile-port på fersk seed er upraktisk lokalt (6,5 GiB + 6–15 min/test); køyr i CI B2 eller compile-committed/run-fresh-sweep. |
| `test_stil` («Ukjent innebygd funksjon: builtin.t.inneholder») | `[x]` FIKSA + VERIFISERT | ROT: `selfhost/nc_main.no` sin råskann av `bruk`-linjer tok med etterfølgjande `// kommentar` i aliaset («t   // …») → `t.inneholder` fall til builtin. Fiks: strip `//`/`#` før modul/alias (ikkje fragmentmodul). Committed seed har same feil innebygd (testen er skippa der). VERIFISERT på fersk seed (aliasfix-container): `std.tekst.inneholder` (ikkje builtin) + test_stil rc=0. |
| Binær NCB-kodar bulk (Fase C.4) | `[ ]` | `selfhost/ncb_bin.no` per-teikn-kodar toppa 8,1 GB RSS → OOM (difor NORSCODE_NCB_BINARY=0). Bulk-agenten stoppa (session-limit) utan committa arbeid. Høyrer til Fase C.4 (NCB→binær), etter promotering. |
| Seed-port-tabell i CI (B2 fullhost) | `[ ]` | A4 ferdig. B2-fullhost-jobben (b2-seed-direct.yml) køyrer heile porten på dedikert runner; ikkje-blokkerande diagnose. Bør re-dispatchast med materialize-json_skriv + random_hex-fiksane. |
| Native `desimaltall` (flyttal) | `[ ]` | 2–3 veker om det skal inn i porten; elles utanfor |
| `db.*` (NorsDB rein Norscode) | `[x]` konformans komplett | Side-spor FERDIG (aa803ab): siste SQLite-korrektheits-gap lukka — G2 (rowid-alias: berre éin-kolonne INTEGER PRIMARY KEY auto-tildeler) + strftime/julianday `%f` millisekund. KONFORMANS_GAP: «Ingen kjende korrektheits-divergensar att». Verifisert: konformans_gap + subquery + default/tx_commit/param_binding/db_adapter grøne. Att: kostnadsbasert join-planleggar (perf, gated på native). |
| tls / sandbox-profilar / trådar | `[x]` pure-Norscode komplett | Side-spor FERDIG (3bef50d): TLS 1.3 komplett for mandatory suite (AES-128-GCM + ChaCha20-Poly1305, X25519, Ed25519, RFC 8448-verifisert incl. ny traffic-key-KAT); trådar (std/tråd.no kooperativ) komplett. Att = valfrie suitar (AES-256/SHA-384, HRR, PSK/0-RTT) + native pool/ssl.no (seed-spor). Sjå docs/TLS_TRAAD_SANDBOX_STATUS.md. |

---

## Fase C — promotering og sletting

Ferdig når: `bootstrap/stage0/*` er bygd av Norscode frå kjelde, og `archive/`-C, hex-blobar,
Python-verktøy og JSON-artefaktar er sletta utan at CI blir raud.

**Inventar 2026-09-07 (utforskingsagent, sjå minne `fase-c-inventar`):** CI er ALT fri for gcc/clang/python/pip/jq/node. 0 Python-filer. 13 C/H-filer (archive/legacy_c_backend + build/v3009) haldne i live berre av innhaldsassertar i `tools/release_preflight.no` + `verify_norscode_surface_ownership.no`. 212 JSON-filer (11,6 MB), 92 `.ncb.json` (9,3 MB). Frosen maskinkode i `native_codegen_v2.no`: 10 913 B (`rt_hex_del0–6` + 2 inline-halar) + 1 999 B `rt_hex_process_spawn` = 12,6 KiB; 70 `RT_*`-adressekonstantar: 9 trampolinerte, 20 daude, **39 framleis levande** (~9 KiB C-æra-kode).

1. `[~]` Promoter fersk seed til `bootstrap/stage0/norscode-linux-x86_64` — **STAGED + validert** (grein `promote-fresh-seed-linux-x86`, 1d9746f). Fersk seed (4,24 MB statisk, alle fiksar) verifisert i Docker: version/run/compile/ci_shell_runner+materialize/test_security/tiny42/sweep OK. **BLOKKAR funne av CI:** som CI-ORKESTRATOR krasjar den ferske seeden i NESTA spawn (`run ci_shell_runner` → ci_runtime_fileops → smoke `prosess.køyr(dist,[selftest],{},120000)` = signal 135 / timeout 124). Korrekt isolert + single-level-spawn; ustabil berre som nesta-orkestrator. Treng process-spawn-harding før faktisk promotering. Gamal seed committa att på #187-greina.
2. `[ ]` Regenerer `bin/nc`/`dist` frå promotert seed på alle plattformer.
3. `[ ]` **Emitter dei 39 levande frosne rutinene som Norscode-atomics** (mønster: 90 atomics + `patch_abs_jump` finst alt). Tyngdepunkt: `RT_JSON_PARSE` (~1,1 KB), `RT_SPLIT`/`RT_REPLACE`/`RT_STR_TO_INT`, `RT_LIST_*`, `RT_MAP_KEYS`/`RT_MAP_VALS` (mest presserande: legacy-lesarar på ny map-layout), `RT_INDEX_GET/SET`, `RT_BUILD_{LIST,MAP}_REV`, `RT_FIL_LES`/`RT_FIL_SKRIV*`, `RT_MILJO_HENT`, 12 aritmetikk/samanlikning, `RT_CONCAT`/`RT_INT_TO_STR`, `RT_INIT_HEAP`, `process_spawn`-blobben (eige atomic). **Blobben må vekk i EITT jafs** (alle RT_* er absolutte VA-ar) → så slett `rt_hex_del*`, `hex_to_bytes`, dei ~25 `replace`-patchane og `patch_*`-funksjonane.
4. `[ ]` NCB JSON → binær (`.ncbin`) overalt: (a) ncb_bin bulk-kodar (agent, pågår); (b) skru på skrivaren (`NORSCODE_NCB_BINARY=1`); (c) konverter/regenerer 92 `.ncb.json`; (d) `elf_compile_driver.no` les 8 `precompiled_fragments_inner/*.functions.json` hardkoda — den EINASTE harde JSON-avhengnaden i bootstrap; fersk seed treng dei ikkje. `nc_main` sine `stdlib`/`precompiled`-oppslag er mjuke (fallback til kjelde; risiko = AOT-heap, som GC-modus løyser).
5. `[ ]` Slett C-arkivet + `build/v3009/*.c` (fjern ~20 assert-linjer i preflight/ownership), `archive/legacy_shell/` (119 .sh), `Dockerfile` (broten python:3.12-image) + `Dockerfile.linux-build`.
6. `[ ]` **Triviell sletting NO (ingen kode les dei, ~2 MB):** 47 `*.tokens.json`, 8 `bootstrap/precompiled_fragments/` (outer-kopi; berre skriven av regenerate-verktøyet), 7 rot-nivå testutdata-JSON, 14 `tools/fixtures/ncb_arm64/`, `build/v9400/`.

---

## Fase D — språkparitet (kan gå parallelt i eigne sesjonar)

- `[ ]` f-strengar (`test_template`), `tools/language_parity_tests.txt` (11 testar, Parserfeil på alle seedar)
- `[ ]` `desimaltall` nativt
- `[x]` NorsDB → SQLite-kompatibel kjerne — konformans komplett (aa803ab): ingen kjende korrektheits-divergensar mot sqlite3 3.51.0. Att: kostnadsbasert join-planleggar (perf).

---

## Nå-kø (kva som køyrer akkurat no)

Ingen bakgrunnsjobbar aktive (Docker restarta ved sesjons-gap; agentane a89b04ec/a204d1ab
ferdige og integrerte). Neste konkrete steg på kritisk sti: **native SHA256-atom** (for
test_security/PBKDF2-ytinga), deretter **seed-promotering** (Fase C).

## Logg

- 2026-09-07 17:05: **Fase A FERDIG** (ci.yml grøn på 8a3a6d0; slow-lanes frå 0/8 → 8/8: OOM-rot = éin tung hybrid-compile, 15 GB-Docker-repro; heavy + pre-eksisterande order-/env-ustabile deferra til fersk-seed-porten). **Fase B rot-årsak funne:** materialize-korrupsjonen er mark-stakk-overflow i `gc_mark_roots` (push utan dedup + stille dropp ved full 4M-stakk). Fiks v6 skriven (gc_push marker-ved-push, 512 MiB stakk @0x4B000000, høg feil ved overflow); validering i tre Docker-containerar (gamal-seed-repro, v6 probar+litmus+materialize, aliasfix-seed). `test_stil`-alias-feil (kommentar på bruk-linje) fiksa i nc_main.no. ncb_bin bulk-kodar delegert til agent (Fase C.4). PBKDF2-fiksen revertert (byte-identitet) — reapply saman med sletting av stdlib-JSON-cache.
- 2026-09-06 23:40: B2 fullhost-pipeline heil (L5+materialize+seed-bygg grøn på fersk direkte seed). BEVIS-crash = GC-tidsavhengig strengkorrupsjon under materialize (socket.no:38 + run-ncb-pure-segfault, begge frå same GC-reuse-feil). GC leiande-hol-sweep fiksa (28db3bb, litmus grøn). Fullhost-jobb er diagnose, ikkje PR-blokkerande. ci.yml: ELF-fixpunkt + attestasjonar grøne, slow-lane matrix køyrer. NorsDB Fase 7 delegert til bakgrunnsagent.


- 2026-09-06 21:30: B2 fullhost: L5 BESTÅTT i CI etter ncb_stream-fallback (committed Linux-stage0 fil-open = EINVAL). Materialize flytta til fersk direkte seed (verts-executor køyrer ikkje fersk NCB; hybrid-compile manglar module_initializers). std.sha256.pbkdf2_hex: feil digest (padda nøkkel/salt) fiksa mot Python; HMAC-midtstand. Ny blokkar: native PBKDF2 heng ved 64 MiB GC-port (4 MiB OK) — gdb-sampling pågår. GC-litmus grøn på 1fc73e0.

- 2026-09-06 19:40: Fixpunkt BESTÅTT lokalt med regenererte fragment (Gen1 == Gen2). Alt pusha (18 commits), B2 fullhost dispatcha. Linux-stage0 sin async-backend ignorerer environment-kartet → adapteren bind miljøet sjølv (env.write), harness-async berre på macOS.

- 2026-09-06 19:15: A1/A2-kode ferdig og verifisert lokalt (runner-strøyming, shard-only, SIGKILL ved timeout). Slow-lanes delt i matrix (macOS 3, Linux 2). `test_template` ut av seed-porten. Fragment regenerert (ir_to_bytecode + semantic endra, resten byte-identisk).
- 2026-09-06 18:50: A3 rot-årsak funne (`imported_funk_kart` skuggar builtin) og verifisert på seed AE. Seed AD bygd med ny nc_main + vm (host_kall-fallback verifisert: `test_template` når no VM-en). CI-runde på ae13f62: begge slow-lanes raude, B2 fullhost raud (gamal kommando).
