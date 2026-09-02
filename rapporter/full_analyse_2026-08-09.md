# Full analyse: Norscode-selfstendig-v2

**Dato:** 9. august 2026
**Analysert av:** Claude (Cowork)
**Omfang:** Hele arbeidsmappen `Norscode-selfstendig-v2` på grein `krypto-tls-primitiver`
**Forrige analyse:** [full_analyse_2026-08-07.md](full_analyse_2026-08-07.md) — denne er en oppdatering med vekt på det som har flyttet seg på to dager.

---

## 1. Sammendrag

Norscode er et selvutviklet norsk programmeringsspråk med egen kompilator, bytecode-VM, standardbibliotek, testrammeverk, pakkesystem, database (NorsDB), LSP/VS Code-støtte og release-løype for fire plattformer. Kjernemålet — at aktiv utvikling, bygg, test og release skal kjøre **uten Python, C eller shell-skript** — er fortsatt reelt oppnådd på filnivå.

De to dagene siden forrige analyse har vært **intenst produktive på ett spesifikt felt**: den ekte blokkeringen for Milepæl B2 («rene native ARM64-bygg uten GCC/OpenSSL»). Diagnosen fra 8. august står fast — AArch64-kodegeneratoren manglet en full-host runtime; den var bare en liten heltalls-AOT uten heap, strenger, lister, kart, sockets eller TLS. Det som har skjedd er at **denne runtimen nå i praksis er skrevet**. Gjennom en fasedelt plan (`docs/SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md`, fase 0–8) er hele NcVal-runtimen portert til AArch64 og verifisert kjørende på ekte Apple Silicon: heap via mmap, strenger (UTF-8), lister/kart, ekte kall og rekursjon, fil-I/O, full JSON (stringify + rekursiv-descent parse), socket-ABI, mutasjon (INDEX_SET, `legg_til`), modul-globaler, unntak (try/catch via setjmp/longjmp-stil handler-stakk), og en voksende «builtin-hale».

Dette er reelt bevegelse på **den ene mest fundamentale gjenværende blokkeringen** i hele selvstendighetsløypa. Men to ærlige forbehold gjelder, og de er godt dokumentert i prosjektets egne planer: (1) en **arkitekturgrense** er oppdaget — operand-stakken er register-basert (x19–x24, maks dybde 6), så kart-literaler >3 par og liste-literaler >6 elementer feiler å kompilere; full `nc_main` bruker slike literaler mye, så dette krever operand-spilling til minne, og det er nå trolig den største enkeltblokkeringen igjen. (2) De to siste stegene som **formelt** lukker B2 — å kjøre hele `nc_main` (3458 linjer) gjennom codegen, og den avsluttende Linux-ARM64-CI-attestasjonen som slipper `/usr/bin/gcc` — kan **ikke** kjøres på denne macOS-verten. Codegen-arbeidet (alle opcodes) er gjort; det som gjenstår er ressurskrevende gjennomkjøring og et eksternt CI-steg.

Repo-hygienen er uendret fra forrige analyse — og det er ikke et kompliment: **ingen** av de konkrete ryddeanbefalingene fra 7. august er utført. `build/` er fortsatt 7,7 GB, de sju utdaterte rotdokumentene ligger der (inkludert filer som beskriver en Python-arkitektur som «production ready»), skrapfilene `-o` og `spm.txt` står igjen, og de ti «visjonsmappene» og de tre bak-mappene er urørt. Det er forståelig — energien har gått til B2-codegen, som er langt viktigere — men punktene står fortsatt åpne.

## 2. Nøkkeltall

| Måltall | 7. aug | 9. aug | Endring |
|---|---|---|---|
| Total diskbruk (arbeidsmappe) | ~8,2 GB | ~7,9 GB | ↓ (build litt mindre) |
| `build/`-katalog | 7,9 GB | 7,7 GB | ~uendret |
| Filer totalt (utenom `.git`) | 36 691 | 36 336 | ↓ 355 |
| Aktive `.no`-kildefiler (utenom `build/`/bak/`archive`) | ~2 480 | 2 491 | ↑ litt |
| Testfiler (`tests/*.no`) | 1 040 | 1 079 | ↑ 39 |
| Commits siste 7 dager | — | **326** | svært høyt tempo |
| Commits siden 7. aug 00:00 | — | 3 (B2-codegen + revert) | — |

**Aktiviteten er høy og fokusert.** 326 commits på sju dager er mye for et énpersonsprosjekt; hovedsporet er ARM64-codegen og den parallelle strømmede/hybride byggeflaten for Linux-fullhost-kandidaten. `.git` i denne worktreen peker utenfor mappen, så forfatterstatistikk er ikke tilgjengelig herfra.

**Dokumentert teststatus** (fra prosjektets egne matriser, ikke uavhengig reprodusert i denne økten): sluttestflaten mot fixed15-kandidaten grønn 878/878 normal + 13/13 slow-lane; Krypto-smoke run 31245505530 grønn 16/16 på linux-x86_64. Disse tallene er verifisert av CI/operatør, ikke av meg i dag — se §8.

## 3. Det som flyttet seg: ARM64 full-host codegen (Milepæl B2)

Dette er analysens hovedfunn og den eneste vesentlige arkitekturendringen siden 7. august.

**Bakgrunn.** `selfhost/native_execution/native_codegen_v2.no` (3077 linjer) er en x86-64-**only** kodegenerator med en håndemittert NcVal-runtime. AArch64-siden (`macho_arm64_codegen.no` + `elf_arm64_emitter.no`) var bare en liten heltalls-AOT: alle verdier som rå 32-bit heltall i register, maks stakkdybde 4, ingen heap/strenger/lister/kart. Derfor brukte ARM64-TLS-attestasjonskandidaten fortsatt `gcc` på `build/v3009/native_candidate_gc.c` med OpenSSL — den dokumenterte midlertidige broen. En rein ARM64 full-host var ikke produserbar før runtimen var skrevet.

**Fremdrift, fase for fase (alle verifisert kjørende på ekte macOS ARM64 via `nc bygg-native --target macos-arm64`, som bygger *og* kjører Mach-O; Linux-ELF arver samme instruksjonsstrøm):**

| Fase | Innhold | Status | Litmus (exit = VM) |
|---|---|---|---|
| 0 | Grunnmur + verifikasjonssløyfe | Ferdig | fixture → 11 |
| 1 | `skriv`(heltall) + write-syscall | Ferdig | `-123450` |
| 2 | mmap-heap + NcVal-maskin (operand-stakk x19–x25, locals i heap) | Ferdig | integer_ops 11, math 49 |
| 3 | Full strengruntime (concat, slice, find, replace, case, trim, UTF-8) | Ferdig | str_ops 16 |
| 4 | Liste/kart-runtime (BUILD_LIST/MAP, INDEX_GET, finnes_nøkkel) | Ferdig | common_runtime 25 |
| 5 | Ekte `bl`/`ret`-kall + rekursjon (BFS-reachability, prologue/epilogue) | Ferdig | fakultet(5) → 120 |
| 6 | Fil-I/O + miljø + **full JSON** (rekursiv stringify + parse) | Ferdig | fileio 18, json_container 37, json_parse 142 |
| 7 | Socket-ABI (8 builtins, OS-parametrisert syscall) | Socket-ABI ferdig | socket_loopback 6 |
| 8 | Full-host `nc_main` + TLS → **lukker B2** | Pågår | se under |

**Fase 8 — alle kjente opcode-gap er lukket** (INDEX_SET, `legg_til`, modul-globaler via reservert x25 + `module_initializers`, og try/catch via en 256-record handler-stakk). Alt verifisert på host (`arm64_index_set` 241, `arm64_legg_til` 35, `arm64_globals` 109, `arm64_try_catch` 53). **Builtin-halen** er påbegynt: 327 distinkte uimplementerte builtins ble målt over 385 førbygde NCB-er, tungt frontlastet. Landet så langt: `tekst_fra_heltall`/`til_tekst`, `heltall_fra_tekst`, `type`, `nøkler`, `verdier`, `fjern_siste` (batch 1–2), og — de to nyeste commitene, 8.–9. august — `split`, `join`, `fjern_nokkel` og `json_parse_raw` som ekte bl-kalte runtime-rutiner.

**To ærlige blokkeringer igjen (begge dokumentert i planen):**

1. **Arkitekturgrense — operand-spilling (oppdaget 9. august).** Operand-stakken er register-basert (x19–x24, maks dybde 6 siden x25 er globals-base). Derfor feiler kart-literaler >3 par (trenger 8 operander) og liste-literaler >6 elementer med «operand-stakk full». Full `nc_main` bruker store literaler utstrakt, så dette krever spilling til stakk/heap-slot når dybden overstiger registerbudsjettet — et substansielt eget steg, «trolig den største attståande blokkeringa (større enn einskild-builtins)». En fallgruve verdt å merke: et mislykket `bygg-native` etterlater **ingen** binær, så en kjøring gir bash exit 127 — det ser ut som en runtime-feil, men er en kompileringsfeil.

2. **De to eksterne stegene som formelt lukker B2.** (a) Å kjøre hele `nc_main` (3458 linjer) gjennom codegen for å avdekke restgap ble SIGKILL-et på denne verten (compile = 137, minnepress) — trenger en maskin med mer minne eller CI. (b) Den avsluttende `selftest`-attestasjonen som slipper `/usr/bin/gcc` **kan ikke** kjøres på denne macOS-ARM64-verten; den krever ekte ARM64-Linux-CI. Dette er det ytre steget som formelt lukker B2.

**Vurdering:** Dette er ekte, verifisert kjerneframgang på det mest fundamentale gjenværende punktet. Ærligheten i planen er fortsatt prosjektets styrke — den skiller tydelig mellom «codegen ferdig og verifisert på host» og «B2 formelt lukket», og markerte operand-spilling som ny hovedblokkering samme dag den ble funnet i stedet for å pynte på fremdriften.

## 4. Parallelt spor: strømmet Linux-fullhost-kandidat (x86-64)

Utenfor ARM64-arbeidet ligger det en ikke-committet arbeidsflate i `selfhost/nc_main.no` (+399 linjer), `selfhost/native_execution/native_codegen_v2.no` (+230) og en rekke `tools/*_candidate.no`. Sporet handler om å bygge Linux-fullhost-kandidaten fra kilde uten at AOT-heapen renner tom under materialiseringen av hundrevis av modulfunksjoner: nye hjelpere `nc_parse_import_ncb`, `nc_materialiser_import_ncb` og en lineær JSON-serialiserer (`nc_json_lineær*`) som strømmer NCB-utdata én funksjon om gangen i stedet for å bygge hele funksjonskartet i minnet. Dette henger direkte sammen med B2-restansen «løs 8,7 GiB-RSS-toppen ved isolert/strømmet modulmaterialisering». Commit-loggen siste uke er full av dette (`stream NCB output one function at a time`, `verify content-addressed hybrid root caches`, `prefer verified source bundle in hybrid builds`). Den nye `docs/CODEGEN_MODULE_GLOBALS_JSON_GATE_RECIPE.md` dokumenterer en subtil leveringssti: codegen kjører fra en bundlet/embedded NCB, ikke fra kilde, så en kildeendring må leveres via `extract_source_function_candidate` + `overlay_ncb_function_candidate` på en hex-kapabel host.

Dette er reelt arbeid mot samme milepæl (rene native bygg), men på x86-64-siden og fortsatt ucommittet — verdt å lande eller stashe bevisst, så arbeidstreet ikke bærer 918 endrede linjer på ubestemt tid.

## 5. Status mot selvstendighetsmålene

Det store bildet er uendret fra 7. august, med B2-sporet som eneste vesentlige bevegelse:

- **Milepæl A (stabiliser generasjonen): stengt.** Generasjon G-A frosen på commit `52593ad9` (grein `main`); Windows-attestasjonen mottatt og verifisert commit-bunden (GitHub-run 31219997969). Uendret.
- **Milepæl B (rene native bygg): B2 er det aktive sporet.** B1 (Linux x86-64), B3 (ABI-restanser: Windows-tråd, freestanding atomics, chmod/rmdir/range-read) og B4 (samlet plattformbevis) står åpne. B2-codegen er som beskrevet i §3 — codegen ferdig på host, formell lukking venter på operand-spilling + ekstern CI.
- **Milepæl C (release/pakkeflyt): åpen**, kan gå parallelt med B.
- **Milepæl D (rein TLS/krypto): godt i gang.** D1-primitivene (`sha512`, `hkdf`, `x25519`, `ed25519`, `tls13_keyschedule`) er `[x]` — trippel-runtime-verifisert. D2 (TLS 1.3-tilstandsmaskin, X.509-kjede for Ed25519/RSA/ECDSA, differensial-KAT, parser-fuzzing) er i hovedsak inne og CI-attestert (Krypto-smoke 16/16), men flere punkter står `[~]` til de kjøres over native socket-ABI og live-interop mot OpenSSL-adapteren — nettopp der ARM64 fase 7–8 kobler inn. D3 (konstanttidsrevisjon + ekstern sikkerhetsrevisjon + standardbyte) er ikke startet og er det lengste enkeltløpet.
- **Milepæl E (Linux-drift + kvalitetsporter): åpen.**
- **Milepæl F (endelig godkjenning): åpen** — avhenger av A→B→F som kritisk sti.

Sluttplanen (`docs/SELVSTENDIGHET_SLUTTPLAN.md`) er oppdatert i arbeidstreet med den dypere B2-diagnosen; `docs/MILEPÆL_B_RUNBOK.md` har fått et avsnitt om at full-host-runtime-mangelen endrer rekkefølgen (den kommer før D2-socket-spørsmålet). Begge endringene er ucommittet.

## 6. Kodekvalitet og testflate

Uendret vurdering: kildekoden er konsekvent i nynorsk-stil helt ned i runtime-lagene, og testflaten er stor og reell (nå 1 079 testfiler, +39 siden 7. august — mange nye ARM64-codegen-fixtures som `arm64_split`, `arm64_join`, `arm64_fjern_nokkel`, `arm64_tid_ms`, `native_codegen_module_globals_probe`). ARM64-codegen-arbeidet har en eksemplarisk verifikasjonsdisiplin: hver fase har en konkret «kjør-og-verifiser-port» der exit-koden fra den native binæren sammenlignes byte-for-byte mot VM-tolken, og disassemblering (`otool -tv`) brukes til å fange subtile enkodingsbugger (dokumentert: to `ldr`/`sub`-immediat-feil funnet slik).

**Det jeg ikke kunne verifisere selv i dag:** testkjøringen. `dist/norscode_native` er en macOS-binær; jeg har ikke kjørt `./bin/nc local-green` eller testflaten i denne økten, så testtallene er prosjektets egne dokumenterte tall. Anbefaling som før: kjør `./bin/nc local-green` lokalt på Mac-en for uavhengig bekreftelse.

## 7. Teknisk gjeld og ryddekandidater — status siden 7. august

**Ingen av ryddeanbefalingene fra forrige analyse er utført.** Konkret, fortsatt til stede:

- `build/` er 7,7 GB (97 % av mappen). Største poster uendret: `nc-module-cache` 1,9 GB, `nc-import-tmp` 729 MB, `v3600` 525 MB, `phase3` 214 MB, `nc-test-cache` 165 MB. Anslagsvis 7+ GB kan frigjøres lokalt (behold de to sporede NCB-bundlene).
- Rotskrap: filen `-o`, `spm.txt` («hva er 2+2?»), `norscode_ai_motor.no` (hardkodet absolutt sti), `norscode-mark.svg` (1,4 MB) — alle fortsatt i rot.
- Utdaterte rotdokumenter: `STATUS_PRODUCTION_READY.md` og `FINAL_SUMMARY.md` (beskriver Python-arkitektur som «100 % PRODUCTION READY»), pluss `SOCKET_IMPLEMENTATION_COMPLETE.md` m.fl. — motsier fortsatt dagens arkitektur og bør arkiveres til `docs/_archive/`.
- Bak-mapper (`tests_bak_alle_moduler_importert_ok/`, `examples_bak_alle_moduler_importert_ok/`, `ai_assistent_bak_v101_files_ok/`) og de ti visjonsmappene (`civilization/`, `sovereignty/`, `evolution/`, …) — alle fortsatt til stede.

Dette er ikke en forverring — filtellingen gikk faktisk ned 355 — men punktene står uendret åpne. Én ny, liten gjeldspost: 918 linjer ucommittet arbeid i arbeidstreet (§4) som bør landes eller stashes.

## 8. Anbefalinger i prioritert rekkefølge

1. **Land eller stash det ucommittede B-sporet.** 918 endrede linjer i `nc_main.no`/`native_codegen_v2.no` + nye candidate-verktøy bør enten committes bak et tydelig delmål eller stashes, så arbeidstreet er rent før neste codegen-inkrement.
2. **Angrip operand-spilling som neste ARM64-steg.** Planen identifiserer dette som den største gjenværende blokkeringen for full `nc_main` — større enn enkeltbuiltins. Uten spilling stopper full-host-kompileringen på store literaler uansett hvor mange builtins som portes.
3. **Sett opp / bruk ekte ARM64-Linux-CI for de to eksterne B2-stegene.** Full-`nc_main`-gjennomkjøring (trenger mer minne enn denne verten) og drop-gcc-`selftest`-attestasjonen er det som formelt lukker B2 — begge må gå på host/CI, ikke i sky-økt eller på denne Mac-en.
4. **Rydderunden fra 7. august, fortsatt utestående** (lav innsats, høy klarhet): slett cache-/tmp-mapper under `build/` (~7 GB), arkiver de sju utdaterte rotdokumentene, fjern `-o`/`spm.txt`, flytt bak-mapper og visjonsmapper. Dette gjør at repoets fortelling stemmer med virkeligheten for både mennesker og agenter.
5. **Lengre sikt:** følg kritisk sti A→B→F. Etter B: ny komplett `--strict`-port på gjeldende generasjon, og D3 (konstanttidsrevisjon + ekstern sikkerhetsrevisjon) som er det faktiske skillet mot full selvstendighet — bestill den eksterne revisjonen tidlig (4–8 ukers leietid).

## 9. Hva denne analysen bygger på

Analysen er gjort mot arbeidstreet slik det så ut 9. august 2026 på grein `krypto-tls-primitiver`: katalog- og filopptelling (36 336 filer), lesing av de aktive plandokumentene (`SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md`, `SELVSTENDIGHET_SLUTTPLAN.md`, `MILEPÆL_B_RUNBOK.md`, `NORSCODE_SELFSTENDIGHET_PLAN.md`), git-logg for de tre commitene og 326-commit-uketempoet, `git diff` av det ucommittede arbeidet (918 linjer), og verifikasjon av at forrige rapports ryddeanbefalinger fortsatt står åpne. ARM64-codegen-fremdriften er lest fra plandokumentets fase-porter og arbeidsloggen; litmus-exit-kodene er prosjektets egne verifiserte tall på ekte macOS-ARM64. Testtotaler og plattformattestasjoner er prosjektets dokumenterte CI-/operatørtall — jeg kjørte ikke testflaten selv, fordi `dist/norscode_native` er en macOS-binær og de formelle B2-stegene krever mer minne / ekte ARM64-Linux-CI enn denne økten har.
