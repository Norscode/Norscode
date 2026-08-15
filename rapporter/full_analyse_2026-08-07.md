# Full analyse: Norscode-selfstendig-v2

**Dato:** 7. august 2026 (revidert samme dag mot `docs/NORSCODE_SELFSTENDIGHET_PLAN.md`)
**Analysert av:** Claude (Cowork)
**Omfang:** Hele arbeidsmappen `Norscode-selfstendig-v2` (git-worktree av Norscode-repoet)

---

## 1. Sammendrag

Norscode er et selvutviklet norsk programmeringsspråk med egen kompilator, bytecode-VM, standardbibliotek, testrammeverk, pakkesystem, database (NorsDB), LSP/VS Code-støtte og release-løype for fire plattformer. Prosjektet er uvanlig ambisiøst og uvanlig godt dokumentert til å være et énpersonsprosjekt, og det mest sentrale målet — at aktiv utvikling, bygg, test og release skal kjøre **uten Python, C eller shell-skript** — er reelt oppnådd på filnivå: jeg fant **null** aktive `.py`-, `.c`-/`.h`- eller `.sh`-filer utenfor `archive/`, `build/` og `third_party/`. Selv CI-en på GitHub bruker Norscode-binæren som shell.

Den autoritative statusen står i `docs/NORSCODE_SELFSTENDIGHET_PLAN.md` (arbeidsloggen for selvstendighetsløypa): **71 av 92 punkter verifisert per 6. august 2026, 21 åpne**. De reelle gjenværende blokkeringene er Windows stage0/kandidat-paritet (den ene sanne release-preflight-feilen), ny komplett strict-port etter siste generasjonsskifte, ren Norscode TLS/krypto med ekstern sikkerhetsrevisjon før OpenSSL kan fjernes, GCC/Zig-overgangene i Linux-byggene, og Linux-driftsflaten (systemd/journald/procfs).

Det som trekker ned på repo-hygienen: **(1)** `build/`-katalogen er på 7,9 GB og utgjør 97 % av mappens diskbruk — nesten alt er cache som trygt kan slettes lokalt; **(2)** rotkatalogen inneholder utdaterte statusdokumenter fra juni 2026 som direkte motsier dagens arkitektur (de beskriver en Python-basert serverløsning som «production ready»), pluss en del skrap.

Samlet vurdering: **kjernen er solid og aktiv** (2 184 kildefiler endret siden 25. juli), og dokumentasjonen er uvanlig ærlig — planen åpner avhukinger på nytt når nye bevis avkrefter dem. Repoet trenger likevel en ryddesjau i rot, bak-mapper og build-cache.

## 2. Nøkkeltall

| Måltall | Verdi |
|---|---|
| Total diskbruk (arbeidsmappe) | ~8,2 GB, hvorav `build/` 7,9 GB |
| Filer totalt (utenom `.git`) | 36 691 |
| Aktive `.no`-kildefiler (utenom `build/` og bak-mapper) | ~2 480 filer, ~340 000 linjer |
| Genererte `.no`-filer i `build/` | 6 431 filer, ~2,3 mill. linjer |
| Standardbibliotek (`std/`) | 269 moduler, ~85 000 linjer |
| Selfhost-kjerne (`selfhost/`) | 262 filer, ~62 000 linjer (`vm.no` alene: 7 086 linjer) |
| Runtime (`runtime/`) | 116 filer, ~69 000 linjer |
| Verktøy (`tools/`) | 340 filer, ~40 500 linjer |
| Tester (`tests/`) | 1 040 testfiler, ~42 500 linjer |
| Dokumentasjon | 317 md-filer i `docs/`, 430 totalt |
| CI-workflows | 8 (GitHub Actions, Norscode-eid shell) |
| Stage-0-seeder | 4 plattformer, hash-låst i `SHA256SUMS` + rollback-kopi |

Dokumentert teststatus (fra `docs/STATUS.md`, verifisert 16. juli 2026 av prosjektet selv): 563/563 bestått i normalflaten, 0 feilet, 21 plattformfiltrerte; slow-lane 11/11. Runtime v1-matrisen: 19 av 22 områder stabile (86 %). Stdlib-statusmatrisen: 57 av 61 moduler stabile (93 %); `sikkerheit`, `dns`, `tls_acme` og `domenehost` er fortsatt eksperimentelle.

## 3. Arkitektur

Normalkjeden er kort og konsekvent gjennomført: `.no`-kildekode går gjennom lexer, parser, semantisk analyse og bytecode-generering til **NCB JSON** (Norscode-bytecode), som kjøres av `selfhost/vm.no`. CLI-en `./bin/nc` er en symlink til `dist/norscode_native` — en native binær (6,3 MB Mach-O ARM64 for macOS) med det aktive NCB-bildet innebygd. Bootstrap-kjeden er løst med innsjekkede stage-0-binærer for macOS ARM64, Linux x86_64, Linux ARM64 og Windows x86_64, alle hash-låst med rollback-artefakter.

Standardbiblioteket er svært bredt for et språk på dette stadiet: full kryptoflate i ren Norscode (SHA-256, HMAC, PBKDF2, scrypt med RFC 7914-vektor, Argon2id med RFC 9106-vektor, BLAKE2b, ChaCha20-Poly1305), DNS med DNSSEC-signering, ACME/RFC 8555-flyt testet mot Pebble i CI, mailserver-, domenehost- og brannmurkontrakter, GC med generasjoner og flyttende komprimering, baseline-JIT for ARM64/x86-64, tråder over pthreads, scheduler med kqueue/epoll/IOCP, tensor-/Metal-støtte og en lokal AI-kontrakt (`std.ai`). I tillegg kommer NorsDB (egen database med CRUD, skjema, transaksjoner, indeks og recovery-smoke), pakkesystemet `norspkg` med lockfile og registry, LSP-server med VS Code-utvidelse, og web-rammeverk med FastAPI-lignende scaffolding.

CI-oppsettet er uvanlig: GitHub Actions bruker stage-0-binæren som *shell* (`shell: …norscode-linux-x86_64 run tools/ci_shell_runner.no {0}`), slik at selv CI-steg kjører gjennom Norscode i stedet for bash. Windows-attestasjon er bundet til GitHub build provenance med commit- og binærhash, og `platform_readiness_v3600` nekter å godta krysskompilering som plattformbevis. Dette er en moden og etterrettelig bevismodell.

## 4. Status mot selvstendighetsmålene

Hovedmålet — aktiv flate uten Python og C — er **verifisert oppnådd på filnivå**. Utenfor `archive/`, `build/` og `third_party/` (som kun inneholder SQLite) finnes det ingen `.py`-, `.c`-, `.h`- eller `.sh`-filer i det hele tatt. De 363 `.c`-filene i mappen ligger i `build/` (352, genererte), `archive/` (10, historikk) og `third_party/` (1, SQLite). Fase 1–6 i selfhost-handlingsplanen er avkrysset som fullført, og styringsdokumentene er bevisst forsiktige: `docs/STATUS.md` skiller eksplisitt mellom historiske attestasjoner (CI-kjøring 30788036517 på commit `39f0447` ga `production_ready_all_platforms=true`) og dagens kandidat, som skal rapporteres `false` til nye signerte rapporter foreligger. Den ærligheten er en styrke.

Det som gjenstår ifølge prosjektets egne matriser: ren Norscode TLS/krypto (OpenSSL er fortsatt adapter for TLS 1.3), Windows-thread-backend og freestanding atomics, hard Darwin-minnegrense, preemptiv scheduler, og de eksplisitte fase-5-overgangene (GCC/Zig/OpenSSL-spor for Linux-bygg). 10/10-modenhetsgaten er en produkt-/bevisflate og skal ikke leses som at disse er ferdige — det sier dokumentasjonen selv.

Selvstendighetsplanen (`docs/NORSCODE_SELFSTENDIGHET_PLAN.md`) er det mest presise statusbildet: 71 av 92 punkter verifisert per 6. august, 21 åpne. Fase 1–2 (sannferdige porter, L5b-reparasjon), fase 4 (reproduserbar stage0 med byte-identisk Gen1/Gen2 og attestert Windows-promotering) og fase 6 (yting: 2× varm bygging, 55 % raskere kald bygging, 50 % lavere L5b-minnetopp) er lukket på levende bevis. Åpne står: Windows stage0/kandidat-paritet (kandidaten er 9,5 MB mot stage0s 11,6 MB — et reelt generasjonsavvik som krever ny ekte Windows-attestasjon før promotering), komplett `selvstendighet --strict`/`local-green --strict`/`release-preflight` etter siste generasjonsskifte, native Linux-bygg uten Docker/GCC/Zig-overganger, fase 7 (ren TLS/krypto, ekstern sikkerhetsrevisjon, fjerning av SQLite/OpenSSL/Zig som standard) og to fase-8-punkter (Linux-drift med systemd/journald/procfs og utskifting av de gjenværende Python-kvalitetsportene). Sluttestflaten mot fixed15-kandidaten er grønn: 878/878 normal og 13/13 slow-lane uten uklassifiserte hopp.

**Verifisert ved kjøring — og korrigert etter planlesing:** Jeg kjørte den innsjekkede `bootstrap/stage0/norscode-linux-arm64` (SHA-256 verifisert identisk med `SHA256SUMS`) på en faktisk ARM64-Linux-maskin. Ved første test så det ut som binæren bare kjørte innebygd selvtest og ignorerte alle argumenter. Selvstendighetsplanen dokumenterer imidlertid at dette er et designvalg: «Committed stage0 ignorerer CLI-argument og startar elles standard-selftest» — stage0-seedene styres via miljøvariablene `NORSCODE_CMD` og `NORSCODE_FILE`, slik alle åtte CI-workflowene gjør. Jeg reproduserte dette live: med `NORSCODE_CMD=run NORSCODE_FILE=<fil>` kjørte ARM64-seeden testprogrammet mitt korrekt (`2+2 = 4`). Linux ARM64-seeden er altså funksjonell. Den lille restanbefalingen er å nevne miljøkontrakten i `bootstrap/stage0/README.md` slik at en utenforstående (eller et verktøy) ikke feiltolker selvtest-oppførselen som en defekt — nøyaktig den fellen jeg gikk i først.

## 5. Kodekvalitet og testflate

Kildekoden er konsekvent i stil, med norsk (nynorsk) syntaks og navngivning gjennomført helt ned i runtime-lagene. Testflaten er stor og reell: 1 040 testfiler som dekker alt fra RFC-vektorer for krypto til GC-relokering, TLS-fixtures (testnøklene i `tests/fixtures/tls/` er nettopp det — testfixtures, ikke lekkede hemmeligheter), Windows-prosessattestasjon og web-kontrakter. Testløperen har signaturbasert NCB-cache, isolerte arbeidsmapper per kjøring og egne timeouts for slow-lane — dette er infrastruktur på nivå med modne språkprosjekter.

Det jeg ikke kunne verifisere selv: selve testkjøringen. Arbeidsmaskinen min i denne økten er ARM64-Linux, `dist/norscode_native` er en macOS-binær, og ARM64-seeden kjører som nevnt bare selvtest. Testtallene i rapporten er derfor prosjektets egne dokumenterte tall, ikke uavhengig reprodusert av meg. Kjør gjerne `./bin/nc local-green` lokalt på Mac-en for å bekrefte at flaten fortsatt er grønn.

## 6. Teknisk gjeld og ryddekandidater

**Build-katalogen (7,9 GB) er den store diskspiseren.** Den er allerede gitignorert (kun to NCB-bundler er sporet), så dette er et rent lokalt diskproblem. De største postene: `nc-module-cache` 1,9 GB, `nc-import-tmp` 729 MB, `v3600` 526 MB, `phase3` 214 MB, `cache` 164 MB, `nc-test-cache` 159 MB, tre `windows-repro-*`-mapper på ~70 MB hver, pluss 381 loggfiler og 6 431 genererte `.no`-filer. Anslagsvis 7+ GB kan frigjøres ved å slette cache- og tmp-mapper (behold `build/bootstrap_compiler_bundle.ncb` og `build/native_elf_compiler_bundle.ncb` som er sporet).

**Rotkatalogen trenger rydding.** Konkrete funn: en fil som bokstavelig talt heter `-o` (feilskrevet kompilator-output med bytecode-JSON); `spm.txt` med innholdet «hva er 2+2?»; en rekke test- og demofiler (`test_*.no`, `norscode_*_test.db`, `sessions_demo.json` m.fl.) som hører hjemme i `tests/` eller `examples/`; og `norscode_ai_motor.no` (186 KB) med hardkodet absolutt sti til `/Users/jansteinar/Norscode AI/prosjekter/NorscodeAIKernel/` — ikke portabel og bør flyttes til det prosjektet den tilhører. `norscode-mark.svg` på 1,4 MB i rot bør komprimeres eller flyttes til `docs/assets/`.

**Utdaterte dokumenter motsier dagens arkitektur.** `STATUS_PRODUCTION_READY.md` og `FINAL_SUMMARY.md` (datert 14. juni 2026) beskriver en Python-basert HTTP-serverarkitektur som «✅ 100% PRODUCTION READY» — stikk i strid med dagens Python-frie flate. Sammen med `SOCKET_IMPLEMENTATION_COMPLETE.md`, `PURE_NORSCODE_SOLUTION.md`, `INTEGRATE_NATIVE_TCP.md` og de to `NORSCODE_VS_*`-sammenligningene er dette historiske arbeidsnotater som etter prosjektets egen regel («historiske filer skal ligge i `docs/_archive/` eller `archive/`») bør arkiveres. Slik de ligger nå, vil en ny leser (eller en AI-agent) kunne trekke feil konklusjoner om arkitekturen.

**Bak-mapper og småskrap.** `tests_bak_alle_moduler_importert_ok/` (380 filer), `examples_bak_alle_moduler_importert_ok/` (65) og `ai_assistent_bak_v101_files_ok/` (15) er manuelle sikkerhetskopier som git allerede ivaretar — kandidater for sletting eller `archive/`. I tillegg: 36 `.DS_Store`-filer (legg gjerne til i `.gitignore` hvis de ikke alt er der) og `.norscode/` med gamle `test-shutil-*`-rester.

**De «visjonære» mappene.** Ti toppnivåmapper (`civilization/`, `sovereignty/`, `evolution/`, `convergence/`, `ecosystem/`, `realization/`, `hardening/`, `autonomous/`, `intelligence/`, `validation/`) inneholder til sammen ~30 filer / ~9 000 linjer med navn som `autonomous_execution_singularity.no`, `autonomous_execution_immortality.no` og `autonomous_execution_transcendence.no`. Dette ser ut som rester fra AI-genererte utforskningsøkter. Om de har verdi, fortjener de én samlet mappe (f.eks. `experiments/`) med en README; om ikke, er de støy som gjør toppnivået uoversiktlig.

## 7. Anbefalinger i prioritert rekkefølge

Først, det som gir mest igjen for minst innsats: slett cache-/tmp-mappene under `build/` og frigjør ~7 GB. Deretter arkivér de sju utdaterte rotdokumentene til `docs/_archive/` og fjern skrapfilene (`-o`, `spm.txt`, løse test-/demofiler i rot) — det gjør at repoets fortelling stemmer med virkeligheten for både mennesker og agenter. Dokumentér miljøkontrakten (`NORSCODE_CMD`/`NORSCODE_FILE`) i `bootstrap/stage0/README.md` slik at seedenes selvtest-oppførsel ikke feiltolkes. Fjern eller arkivér bak-mappene og de ti visjonsmappene. Til slutt, på lengre sikt: følg planens egne 21 åpne punkter i prioritert rekkefølge — attestert Windows-promotering som lukker stage0/kandidat-avviket og den ene release-preflight-feilen, deretter ny komplett strict-port, native Linux-bygg uten overgangsverktøy, og fase 7-løftet (ren TLS/krypto pluss ekstern sikkerhetsrevisjon) som er det som faktisk skiller dagens status fra full selvstendighet på alle plattformer.

## 8. Hva denne analysen bygger på

Analysen er gjort mot mappen slik den så ut 7. august 2026: full katalogopptelling (36 691 filer), lesing av styringsdokumentene (README, STATUS, MODENHET_10_10, SELFHOST_HANDLINGSPLAN, NORSCODE_SELFSTENDIGHET_PLAN, LANE_MAP, ROADMAP, CHANGELOG m.fl.), stikkprøver i kildekoden (`selfhost/`, `std/`, `compiler/`, CI-workflows), og faktisk kjøring av den innsjekkede Linux ARM64-binæren — både med og uten miljøkontrakten `NORSCODE_CMD`/`NORSCODE_FILE`, som bekreftet at seeden er funksjonell. Testtall og plattformattestasjoner er prosjektets egne dokumenterte tall der jeg ikke kunne kjøre dem selv (macOS-binæren kan ikke kjøres i denne økten). Git-historikk var ikke tilgjengelig fordi mappen er en worktree med `.git`-peker utenfor den tilkoblede mappen.
