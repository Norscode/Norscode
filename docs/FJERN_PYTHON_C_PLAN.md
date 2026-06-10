# Plan: fjern C og Python

Operativ avhukingsplan for aa fjerne C og Python fra repoets aktive flate, normal utvikling, bygg, test, CI og release.

Planen bygger paa [SELFHOST_HANDLINGSPLAN.md](SELFHOST_HANDLINGSPLAN.md) og [SELVSTENDIGHET_PLAN.md](SELVSTENDIGHET_PLAN.md).

## Maal

Norscode skal kunne utvikles, bygges, testes og publiseres uten C- eller Python-kilde i normal flyt.

Normal kjede:

```text
.no -> lexer/parser/semantic/bytecode -> NCB JSON -> selfhost/vm.no -> native ELF
```

Ferdig betyr:

- [x] Ingen `.c` eller `.h` i aktiv kildeflate.
- [x] Ingen `.py` i aktiv kildeflate.
- [x] Ingen normale CI-steg bruker `python`, `python3`, `clang`, `gcc`, `cc`, `ncb_to_c` eller C-VM.
  - Attverande unntak er avgrensa til eksplisitte maintainer-workflowar: `regen_bootstrap.yml` og `export-stage0-linux.yml`.
- [x] `./bin/nc run`, `test`, `check`, `bundle`, `bygg-native` og release bruker Norscode-native vei.
- [x] Stage-0 er en verifisert binær seed per plattform, ikke en C-kilde som bygges i daglig flyt.
  - Normal build/CI/release brukar seed-first; C-regenerering er avgrensa til eksplisitt vedlikehaldsmodus.
- [x] Historikk er enten slettet eller samlet i `archive/` med tydelig legacy-merking.
  - Legacy C-host/runtime ligg no i `archive/legacy_c_backend/`; lokal/generert `bootstrap/maint/c/` er berre maintainer-regen-output og ikkje committed normalflate.

## Naavaerende kjente spor

- [x] `tools/maint/c/nc_native_main.c` (flytta til `archive/legacy_c_backend/`)
- [x] `tools/maint/c/nc_runtime_full.c` (flytta til archive)
- [x] `tools/maint/c/nc_runtime_mini.c` (flytta til archive, brukt frå legacy bane ved behov)
- [x] `archive/legacy_c_backend/ncb_to_c.no` (flytta ut frå aktiv flate)
- [x] `archive/c_minimal_vm/` (ligg i `archive/` og er tydeleg legacy)
- [x] `tools/python_dependency_audit.sh` (er no shell-launcher for `.no`-audit)
- [x] `tools/python_free_ci.sh` (er no shell-gate utan Python-avhengigheit)
- [x] `reports/python_dependency_report.txt` (er fjerna/erstatta)
- [x] CI- eller scriptsok etter `python`, `clang`, `gcc`, `cc`, `ncb_to_c`, `.c`, `.h` (inventarisert; attverande treff er vedlikehald/historikk)

## Fase 0: Sann inventory

Maal: vite nøyaktig hva som finnes, hva som brukes, og hva som bare er historikk.

- [x] Lag `reports/no_c_python_inventory.txt` fra `rg --files`.
- [x] List alle `.c`, `.h` og `.py`.
- [x] List alle shell-/CI-referanser til `python`, `clang`, `gcc`, `cc`, `ncb_to_c`, `c_minimal_vm`.
- [x] Merk hver forekomst som `aktiv`, `bootstrap`, `legacy`, `kan slettes`.
- [x] Oppdater denne planen med eventuelle funn som mangler over.

Funn etter inventory:

- Ingen aktive `.py`-filer finst i repoet; `rg --files -g '*.py'` er tom.
- Ingen aktive `.c`/`.h` ligg i `.github`, `tools`, `selfhost`, `bin` eller `bootstrap`; attverande C-kjelder ligg i `archive/legacy_c_backend/`.
- Attverande teksttreff for `clang`, `gcc`, `cc`, `bootstrap/maint/c/` og `ncb_to_c` kjem frå:
  - vedlikehaldsskript under `tools/maint/`
  - eksplisitte vedlikehalds-workflowar
  - historiske/arkiv-merkte dokument
  - plan-/statusdokument som forklarer utfasinga

Verifikasjon:

```bash
rg --files | rg '(\.c$|\.h$|\.py$)'
rg -n 'python3?|clang|gcc|(^|[^a-z])cc([^a-z]|$)|ncb_to_c|c_minimal_vm|nc_runtime|nc_native_main' .github tools selfhost docs bin bootstrap
```

## Fase 1: Stabiliser ekte native runtime

Maal: fjerne den tekniske grunnen til at C fortsatt frister som fallback.

- [x] Fiks ekte Gen1 ELF `bygg_bundle` i `selfhost/elf_compile_driver.no`.
- [x] Fiks native runtime/kodegen for store `fil_les`/`fil_skriv`-operasjoner.
  - [x] Løyst lokalt: typeinfo, dispatch-alias og smoke-testar gjer at `./bin/nc test`, `./bin/nc selfcheck` og `bash tools/verify_selvstendighet.sh` passerer.
- [x] Planlegg/implementer eigentleg runtime-røtt for store fil-I/O-kall (full smoke-verifisering).
  - [x] Legg til `fil_skriv_binary`-alias i runtime-dispatch (`native_codegen_v2.no`).
  - [x] Legg inn smoke-test `tests/test_file_io.no` for `fil_les`/`fil_skriv`/`fil_skriv_binar`/`fil_finnes`.
- [x] Fiks native runtime/kodegen for map/list/json nok til compiler-bundle.
- [x] Fjern midlertidig NCB-kopi i `tools/selfcompile_stage0_elf.sh` naar ekte Gen1 ELF bundle fungerer.
- [x] Fjern host-patching av stage-0 bundle-entry i 6b-kjeda.
  - Entry blir no sett i sjølve bundleren via `NORSCODE_BUNDLE_ENTRY`, både for host-bygd Gen1 NCB og Gen1 ELF → Gen2 NCB.
  - Det tidlegare hjelpeskriptet `tools/patch_ncb_entry.sh` er fjerna.
- [x] Verifiser Gen1 ELF -> Gen2 NCB -> Gen2 ELF uten host-kopi.
  - [x] Linux x86_64 sjølvkompilering er no verifisert i Docker med byte-paritet for både NCB og ELF.
  - [x] Linux CI køyrer denne med `NC_OM6B_RUN_STAGE0=1` og prøver no ekte `NORSCODE_BUNDLE_ARGS` først; ved Gen1 ELF-krasj blir chunk-artifact frå same ELF-køyring brukt vidare utan shell-kopi av NCB.
  - [x] Transitional source-NCB er no chunka i `NC_OM6B_CHUNK_SIZE`-delar (default 64 KiB), slik at `elf_compile_driver.no` kan rekonstruere same NCB utan éi stor `fil_les`.
  - [x] `NORSCODE_OM6B_PRESET=1` via `dist/norscode_native` rekonstruerer no stage-0 NCB korrekt; `NC_OM6B_VERIFY_PRESET=1` er verifiseringsbane for denne preset-løypa.
  - [x] Fallback-løypene med `direct-source-ncb` og `chunked-source-ncb` skriv ikkje full `stage0_elf_passed.marker`; berre ekte intern ELF-bundle/preset-paritet kan produsere stage-0-kandidatmarkør.
  - [x] `tools/selfcompile_stage0_elf.sh` skil no eksplisitt mellom transitional-modusane:
    - `direct-source-ncb`
    - `chunked-source-ncb`
    - denne modusen blir skrive både i logg og `stage0_elf_transitional.marker`
  - [x] `tools/verify_omgang6b.sh` verifiserer no begge transitional-rekonstruksjonsbanene lokalt:
    - direkte source-NCB via `NORSCODE_OM6B_SOURCE_NCB`
    - chunked source-NCB via `NORSCODE_OM6B_SOURCE_NCB_DIR` + `NORSCODE_OM6B_SOURCE_NCB_COUNT`
  - [x] Transitional CI rapporterer no ekte Gen1 ELF exit-kode og lastar ikkje opp stage-0-kandidat utan full paritet-marker.
  - [x] Transitional CI lastar opp `gen1_elf_bundle.log` som eige artefakt når Gen1 ELF faktisk køyrer.
  - [x] Transitional CI lastar opp `gen1_elf_diagnose.txt` med `file`, `readelf`, SHA256, kommando og log-tail for Gen1 ELF.
  - [x] Gen1 ELF-diagnose bevarer no separate modusloggar for Linux-køyringa:
    - `gen1_elf_preset.log`
    - `gen1_elf_source_ncb.log`
    - `gen1_elf_chunked_source_ncb.log`
    - `gen1_elf_bundle.log` som samla logg med exit-kodar
    - `gen1_elf_attempts.txt` som kort indeks over modus, exit-kode og loggfil
    - dette hindrar at ein vellykka fallback overskriv feilloggen frå ekte preset/direct-source-løype.
  - [x] `elf_compile_driver.no` vel no Omgang 6b-modus frå `miljo_hent(...) != ""` i staden for `miljo_finnes(...)`.
    - GitHub-diagnosen viste at Gen1 ELF gjekk til chunked branch også i preset/source-run.
    - Dette snevrar inn feilen til branch-val/runtime-miljøoppslag og fjernar avhengigheita av ein svak `miljo_finnes`-builtin i denne kritiske løypa.
  - [x] Ekte Gen1-førsteforsøk brukar no `NORSCODE_BUNDLE_ARGS` før Omgang6b-flagg.
    - Dette brukar miljøvariablane som allereie fungerte fram til `etter args`/`etter out`.
    - Omgang6b source/chunk-flagg er no berre fallback-løyper.
    - Førsteforsøket går direkte til lokal `bygg_omgang6b_bundle(...)` når `NORSCODE_BUNDLE_ARGS` er sett, utan tekst-samanlikning eller generell args-parser som krasja på Linux.
    - Precompiled-filnamn blir no sendt eksplisitt til helperen, slik at Gen1 ELF slepp `split`/`ends_with`/`slice` under første moduloppslag.
  - [x] Fiksa `heltall_fra_tekst` i native ELF-runtime.
    - `builtin_va(...)` kjenner no att modul-kvalifisert `.heltall_fra_tekst`.
    - `RT_STR_TO_INT()` peikar no til faktisk tekst-til-heiltal-start (`0x4016b0`) i staden for midt i int-til-tekst-rutina.
    - Docker-smoke med `heltall_fra_tekst("42")` byggjer ELF, køyrer og skriv `42`.
    - `tools/verify_omgang6.sh` har eigen `str-to-int` smoke for å låse regresjonen.
  - [x] Fiksa `fil_les` og `lengde` i native ELF-runtime.
    - `RT_FIL_LES()` peikar no til faktisk fil-les-rutine (`0x402150`) i staden for type-wrapperen.
    - `RT_LENGDE()` peikar no til payload-lengd-rutina (`0x4019c0`) i staden for JSON/nummer-parserområdet.
    - Docker-smoke med `fil_les("bootstrap/precompiled/lexer_m1.ncb.json")` + `lengde(...)` skriv `41331`.
  - [x] Fiksa `json_parse_raw("{}")`-entry i native ELF-runtime.
    - `RT_JSON_PARSE()` peikar no til wrapperen på `0x402ac0`, som bygg parser-state frå tekst før den går inn i kjernen på `0x402b00`.
    - Docker-smoke med `json_parse_raw("{}")` går no gjennom og returnerer utan krasj.
  - [x] Fiksa fleire native JSON-delstiar for ikkje-tomt innhald.
    - Docker-smokar med `json_parse_raw("1")`, `json_parse_raw("{\"a\":1}")` og små NCB-liknande objekt går no gjennom.
    - `tests/test_file_io.no` er grøn igjen i Docker.
  - [x] Fiksa modul-renaming for bytecode-kall i bundling.
    - `omdøyp_funksjonar(...)` og lokalvariantane oppdaterer no også `CALL "__main__.…"` inni bytecode, ikkje berre funksjonsnøklar og `module`.
    - Dette hindrar at `selfhost.elf_compile_driver.bundle_modus` hoppar inn i `selfhost.bundler.bygg_omgang6b_bundle` ved namnekollisjonar.
  - [x] Ekte `Gen1 ELF` bundle-args produserer no gyldig Gen2 NCB og Gen2 ELF med byte-paritet.
    - `elf_compile_driver.no` brukar precompiled inner-fragment og chunk-output for å unngå den tidlegare native JSON-hotpathen.
    - `tools/selfcompile_stage0_elf.sh` aksepterer chunk-artifact frå same Gen1 ELF-køyring når prosessen krasjar etter vellykka skriving.
    - Runtime-adressene for `fil_skriv`/`fil_skriv_binar` og halen etter `close` er korrigerte i `native_codegen_v2.no`.
  - [x] Ekte Gen1 bundle-args kjem no til riktig branch før fallback-miljø blir lese.
    - `elf_compile_driver.no` sjekkar `NORSCODE_BUNDLE_ARGS` før `NORSCODE_OM6B_SOURCE_NCB*`, slik at svak missing-env-handtering ikkje vel fallback tidleg.
    - `bootstrap/precompiled/elf_compile_driver.ncb.json` er regenerert frå Norscode-kjelda.
    - Neste opne blokkering: Gen1 ELF krasjar framleis rett etter `bygg_omgang6b_bundle: før første precompiled`, før første modul er ferdig lasta/omdøypt.
  - [x] `elf_compile_driver.no` loggar no chunk-for-chunk i transitional source-NCB-løypa:
    - chunk count
    - kvar `part_XXX.json` som blir lesen
    - akkumulert byte-lengd etter kvar chunk
    - før/etter `fil_skriv` i chunk-løypa
    - før/etter `fil_les`/`fil_skriv` i direkte source-NCB-løypa
  - [x] Korrigerte `_start`/runtime-kontrakt for `envp`: `native_codegen_v2.no` lagrar no `envp` til `HEAP_VA + 8`, som er adressa runtimeen faktisk les i `miljo_hent()`.
  - [x] Korrigerte `_start`-maskinkode for `argc/argv/envp`-lesing: generatoren bruker no faktisk `%r12` som base-register, ikkje `%rsp` etter diagnose-push/pop.
  - [x] Verifisert lokalt etter `envp`-rettinga:
    - `./bin/nc check selfhost/native_execution/native_codegen_v2.no`
    - `./bin/nc verify-omgang6b`
    - `./bin/nc test`
  - [x] Verifisert med objdump på fersk Gen1 ELF at `_start` no emitterer:
    - `mov (%r12), %rdi`
    - `lea 0x8(%r12), %rsi`
    - `lea (%r12,%rdi,8), %rdx`
  - [x] Ingen `bootstrap/precompiled/*.ncb.json` måtte regenererast for denne rettinga, fordi endringa ligg i `selfhost/native_execution/native_codegen_v2.no` og ikkje i modulane som blir lest frå `bootstrap/precompiled/`.
  - [x] `_start` i `native_codegen_v2.no` skriv no diagnosemarkørar for Linux-sporet:
    - `0`: før envp-utrekning
    - `1`: etter envp-lagring
    - `2`: etter stack-align
    - `A`: før `rt_init_heap`
    - `B`: etter `rt_init_heap`
    - `C`: rett før kall til `start()`
  - [x] Hypotese-matrise for Linux-markørar:
    - Ingen markør: krasj før eller i aller første `_start`-instruksjon; sjekk ELF-header, entrypoint og loader/segment-layout.
    - Berre `0`: krasj mellom tidleg `_start` og envp-lagring; sjekk `argc/argv/envp`-adresseutrekning og registerbruk rundt `r12/rdi/rdx`.
    - `01` men ikkje `2`: envp er lagra, men krasj før eller under stack-align; sjekk stack-manipulasjon og skriveadresse for envp-slot.
    - `012` men ikkje `A`: krasj rett etter align og før første runtime-kall; sjekk overgang frå handskriven `_start` til call-reloc/runtime-adresser.
    - `012A` men ikkje `B`: krasj inni `rt_init_heap`; sjekk heap-base, init-rutine og skrivebarheit i data/heap-segment.
    - `012AB` men ikkje `C`: heap-init er ferdig, men krasj før hopp til `start()`; sjekk `main_va`-oppslag og `call_rel32`.
    - `012ABC` og så krasj: inngang til `start()` skjer, så feilen ligg i `elf_compile_driver.no` eller seinare runtime-kall.
  - [x] Diagnosemarkørane er no fjerna frå normal `_start`-flyt etter at Linux/Omgang 6b-sporet vart stabilisert.
    - Markørane var nyttige for å finne envp-/stack-feila, men er ikkje lenger del av vanleg ELF-køyring.
  - [x] Dokumenter runtime-adresse-/ABI-kontrakt for `native_codegen_v2.no` i `docs/NATIVE_CODEGEN_V2_ABI.md`.
  - [x] Native regresjonssmoker dekkjer no også:
    - `type_av(a)` for parameterverdi i brukerfunksjon
    - `a + b` for tekstparametrar i brukerfunksjon
    - Dette er låst inn i `tools/verify_omgang6.sh` via modusane `type` og `add`.
  - [x] `native_codegen_v2.no` behandlar no `type_av(...)` inline og brukar inline dynamisk `+` for vanlege `tekst+tekst`/`heltall+heltall`-tilfelle.
    - `RT_TYPE_OF()` og `RT_ADD()` står att som legacy-adressemarkørar, men er ikkje lenger den primære native-vegen for desse operasjonane.

Verifikasjon:

```bash
NC_OM6B_RUN_STAGE0=1 bash tools/selfcompile_stage0_elf.sh
./bin/nc verify-omgang6b
```

Ferdig naar:

- [x] `ELF self-compile paritet (Linux x86_64)` passerer med ekte Gen1 ELF bundle.
- [x] Ingen paritetstest er avhengig av shell-kopi av NCB som erstatning for ELF-kjoring.

## Fase 2: Gjør stage-0 til binærkontrakt

Maal: stage-0 er en verifisert Norscode-binær, ikke en C-byggvei.

- [x] Lag manifest for `bootstrap/stage0/norscode-*` med SHA256.
- [x] Gjør `tools/build_norscode_native.sh` seed-only i normalmodus.
- [x] Fjern `NORSCODE_BOOTSTRAP_C=1` fra normal buildscript.
- [x] Fjern clang/cc fallback frå normal buildscript.
- [x] Legg inn eksplisitt feilmelding hvis seed mangler.
- [x] Dokumenter hvordan ny stage-0 seed produseres fra Norscode-native pipeline.
- [x] Bruk seed-only i `format-lint-check` i CI.
- [x] Bruk seed-only i dei vanlege macOS/Linux build-jobbane i `ci.yml`.
- [x] Bruk seed-only i Linux release-jobb i `publish.yml`.
- [x] Hold `regen_bootstrap.yml` og `export-stage0-linux.yml` som bevisste maintainer-migrasjonsbaner for `NORSCODE_BOOTSTRAP_C=1`.

Verifikasjon:

```bash
bash tools/build_norscode_native.sh
bash tools/verify_seed_only.sh
```

Ferdig naar:

- [x] Ny maskin kan kjøre `bash tools/build_norscode_native.sh` uten C toolchain.
- [x] CI har en seed-only lane som passerer uten clang/gcc/cc.

Status no:

- Normal utvikling, bygg, test og release er i praksis ferdig flytta over på seed-first / native-first.
- Fase 3 er no verifisert for normalvegen og den eksplisitte maintainer-bana: berre historiske/vedlikehaldsreferansar for C-mellomformatet står att, som avgrensa unntaksflate.

## Fase 3: Fjern C-host og C-runtime

Maal: ingen aktiv C-kilde trengs i repoet.

- [x] Flytt funksjonalitet frå `tools/maint/c/nc_native_main.c` til `.no`.
- [x] Skil runtime-kontrakt frå normalvegen til `.no`/native emitter.
  - `selfhost/native_execution/native_codegen_v2.no` + VM/CLI-bane dekkjer no normal-runtime-vegen.
  - `archive/legacy_c_backend/nc_runtime_full.c` og `archive/legacy_c_backend/nc_runtime_mini.c` er historiske/vedlikeholdsflater for C-brygga.
  - Avklart status per juni 2026:
    - aktiv native runtime-kontrakt ligg i `selfhost/native_execution/native_codegen_v2.no`
    - maintainer-lanen brukar framleis `archive/legacy_c_backend/ncb_to_c.no` + embedded `archive/legacy_c_backend/nc_runtime_mini.c`
    - `archive/legacy_c_backend/nc_runtime_full.c` står att som historisk referanse, ikkje som aktiv normal-runtime
  - Kontrakt-matrise:
    - verdi-layout:
      - native: `NcVal*` med 16-byte `{type,val}` i ELF-runtime
      - maintainer-C: heapallokert `struct NcVal` med tagged union
    - streng/list/map/int/bool/nil:
      - native: dekt i `native_codegen_v2.no` via `RT_*`
      - maintainer-C: dekt i `nc_runtime_mini.c`
    - fil-I/O, miljo, JSON, tekstoperasjonar:
      - native: aktivt brukt og verifisert i ELF-løypa
      - maintainer-C: ikkje den primære utfordringa; brukast via generated-C-banen
    - konklusjon:
      - det reelle restpunktet er ikkje normal native funksjonalitet
      - det er om maintainer-generated-C framleis skal ha ein eigen mini-runtime i det heile
- [x] Fjern `tools/maint/c/nc_runtime_mini.c` frå `tools/maint/c/`.
- [x] Fjern eller arkiver hele `tools/maint/c/`.
- [x] Fjern alle referanser til `bootstrap/maint/c/` i normal docs og scripts. (Attverande referansar er avgrensa til vedlikehaldsskript, vedlikehalds-workflowar og historikk.)
- [x] Fjern C-genererte artefakter frå normal build/release-flow.
  - Attverande faktiske artefaktspor per juni 2026:
    - `bootstrap/maint/c/norscode_generated.c`
  - Attverande bruk er no avgrensa til maintainer-lanen:
    - `tools/build_norscode_native.sh` ved `NORSCODE_BOOTSTRAP_C=1 REGEN=1`
    - `tools/maint/regen_native.sh`, `tools/maint/regen_verify.sh`, `tools/maint/verify_l6.sh`
      - `regen_native.sh` brukar no isolert standard-root `build/maintainer_regen/`; `bootstrap/maint/c/` krev no eksplisitt peiking via `REGEN_ROOT` / `BOOTSTRAP_C_ROOT`
      - `regen_verify.sh` omtalar no eksplisitt dette som deterministisk maintainer-output, ikkje som del av normal byggeflate
      - `verify_l6.sh` brukar no isolert maintainer-output i `build/verify_l6/` i staden for aa skrive aktivt til `bootstrap/maint/c/`
      - `regen_native.sh --rebuild` kan no byggje frå valfri `REGEN_ROOT` via `BOOTSTRAP_C_ROOT`, utan aa vere laast til repoets standard `bootstrap/maint/c/`
    - `tools/maint/ensure_stage0_seed.sh`, `tools/maint/migrate_bootstrap_c_to_stage0.sh`
      - `ensure_stage0_seed.sh` brukar no isolert maintainer-output i `build/ensure_stage0_seed/` i staden for aa lene seg på aktiv `bootstrap/maint/c/`
      - `migrate_bootstrap_c_to_stage0.sh` er no tydeleg merka som maintainer-engangsmigrering til stage0-seed
      - `migrate_bootstrap_c_to_stage0.sh` brukar no isolert maintainer-output i `build/migrate_stage0/` i staden for aa skrive aktivt til `bootstrap/maint/c/`
    - `tools/verify_nc_main_host.sh`
      - sjekkar no regenerert `kompiler.ncb.json` i `build/verify_nc_main_host/`, ikkje `nc_dispatch.c`
      - brukar isolert maintainer-output i `build/verify_nc_main_host/` under verifikasjonen
    - `.github/workflows/regen_bootstrap.yml` (no tydeleg kommentert som maintainer-only seed-maintenance / regen)
    - `.github/workflows/export-stage0-linux.yml` (no tydeleg kommentert som maintainer-only beredskap med isolert local regen-output)
    - `bootstrap/maint/c/README.md` omtalar no mappa som lokal maintainer-output, ikkje som aktiv byggflate, og seier at standard regen-output no ligg under `build/maintainer_regen/`
  - Siste gjenværande C-kjede i maintainer-lanen:
    - `tools/maint/regen_native.sh`
      - bygger `kompiler.ncb.json`
      - køyrer `archive/legacy_c_backend/ncb_to_c.no`
      - skriv `maint/c/norscode_generated.c` i vald isolert root
    - `tools/build_norscode_native.sh` i `NORSCODE_BOOTSTRAP_C=1 REGEN=1`
      - samanføyer `norscode_generated.c`
      - samanføyer `archive/legacy_c_backend/nc_native_main.c`
      - kompilerer dette med `clang`/`cc` til `dist/norscode_native`
    - implikasjon:
      - normal build/test/CI er allereie fri frå denne kjeda
      - det som står att er berre maintainer-bootstrap via generated C som mellomformat
  - Dispatch-leddet er no avvikla frå aktiv maintainer-kjede:
    - fuzzy-oppslag og `nc_fn_builtin_neste_token(...)` er flytta til `archive/legacy_c_backend/nc_native_main.c`
    - host-bridge-innslaga `host_exec_ncb_json` og `host_kall_bygg_bundle` er flytta til host-dispatchen
    - `archive/legacy_c_backend/ncb_to_c.no` genererer no sjølve `NcDispatch`-tabellen inni `norscode_generated.c`
    - `tools/build_norscode_native.sh` byggjer no berre frå `norscode_generated.c` + `nc_native_main.c`
    - `tools/maint/regen_native.sh` regenererer no berre `kompiler.ncb.json` + `norscode_generated.c`
    - `tools/maint/regen_verify.sh` samanliknar no berre desse to artefakta
  - Normal build/CI/release brukar ikkje desse artefakta lenger; punktet som står att er å snevre inn eller avvikle maintainer-lanen vidare.
  - Neste reelle tekniske rest etter dispatch-kuttet:
    - `archive/legacy_c_backend/ncb_to_c.no` genererer framleis heile mellomformatet `norscode_generated.c`
    - dette mellomformatet inneheld tre hovuddelar:
      - embedded `archive/legacy_c_backend/nc_runtime_mini.c`
      - genererte `nc_fn_*`-funksjonar for kvar NCB-funksjon
      - `NcDispatch`-tabellen som no også bur i same fil
    - konklusjon:
      - dispatch-støyen er i praksis fjerna
      - det som står att er sjølve C-mellomformatet og mini-runtimeen, ikkje lenger eit eige dispatch-lag
    - minste neste arkitekturgrep er derfor ikkje meir `nc_dispatch`-rydding
      - det er å vurdere om `nc_runtime_mini.c` og genererte `nc_fn_*` framleis skal vere bootstrap-brua
      - eller om maintainer-lanen må få ein annan kompilerings-/køyremekanisme enn generated C
  - Kartlagt binding mellom generated `nc_fn_*` og `nc_runtime_mini.c`:
    - brukte runtime-symbol i generated C: `69`
    - truleg stage0-kjerne som må bevarast om generated-C-brua blir halde:
      - verdiar/konstruktørar: `nc_nil`, `nc_int`, `nc_bool`, `nc_str`, `nc_map_new`
      - eksekveringskjerne: `nc_push`, `nc_pop`, `nc_store`, `nc_load`, `nc_truthy`, `nc_throw`, `nc_to_str`
      - operatorar: `nc_add`, `nc_sub`, `nc_mul`, `nc_div`, `nc_mod`, `nc_lshift`, `nc_rshift`, `nc_band`, `nc_bor`, `nc_bxor`, `nc_neg`, `nc_eq`, `nc_cmp`
      - container/indexering: `nc_build_list`, `nc_build_map`, `nc_index_get`, `nc_index_set`
    - høgare nivå / meir utskiftbare hjelpelag:
      - fil/miljø: `nc_builtin_fil_*`, `nc_builtin_miljo_*`
      - tekst/JSON: `nc_builtin_*` for `slice`, `split`, `join`, `replace`, `starts_with`, `ends_with`, `contains`, `json_*`, `type`, `heltall`, `tekst_fra_heltall`
      - samlingshjelparar: `nc_builtin_legg_til`, `nc_builtin_fjern*`, `nc_builtin_lengde`, `nc_builtin_nokler`, `nc_builtin_verdier`, `nc_builtin_finnes_nokkel`
      - diverse: `nc_builtin_skriv`, `nc_builtin_feil`, `nc_builtin_exit`, `nc_builtin_bool`, `nc_builtin_desimaltall`, `nc_builtin_n`
    - implikasjon:
      - den minste realistiske runtime-reduksjonen er ikkje å fjerne alt på ein gong
      - ein meir truverdig veg er å skilje ut ein liten stage0-kjerne frå dei høgare nivå-builtinane først
    - foreslått minste neste tekniske reduksjon:
      - steg A: handsam `nc_runtime_mini.c` som to lag:
        - `stage0-kjerne`: verdiar, stack/env, operatorar, list/map/index
        - `hjelpelag`: fil/miljø, tekst, JSON, samlings-builtins, `skriv`/`feil`/`exit`
      - steg B: la maintainer-lanen framleis ha generated C, men med færre embedded builtins i første omgang
      - steg C: berre etter at kjernelaget er isolert, vurder om sjølve C-mellomformatet kan bytast ut
    - praktisk konsekvens:
      - neste gode kutt er ikkje aa slette heile C-brua direkte
      - det er aa gjere mini-runtimeen mindre og meir eksplisitt, slik at stage0-kravet blir skarpare avgrensa
    - status i kode:
      - `archive/legacy_c_backend/nc_runtime_mini.c` er no merka med same todeling:
        - `Stage0-kjerne`
        - `Hjelpelag`
      - dette gjer neste reduksjon mindre uklar, fordi avgrensinga no finst i sjølve legacy-koden, ikkje berre i planen
    - første utfasingrekkefølgje i hjelpelaget:
      - 1. fil/miljø-bridge: `nc_builtin_fil_*`, `nc_builtin_miljo_*`, `nc_builtin_miljo_sett`
      - 2. teksthjelparar: `starts_with`, `ends_with`, `contains`, `split`, `join`, `trim`, `replace`, `chr`, `char_code`, `heltall`, `tekst_fra_heltall`, `lower`, `upper`, `index_of`
      - 3. samlingshjelparar: `lengde`, `legg_til`, `fjern*`, `nokler`, `verdier`, `finnes_nokkel`
      - 4. JSON-lag: `json_parse_*`, `json_stringify*`
      - 5. diverse/stubbar: `skriv`, `feil`, `exit`, `desimaltall`, `openapi_json`
    - status i legacy-koden:
      - `archive/legacy_c_backend/nc_runtime_mini.c` har no eigne seksjonsgrenser for:
        - `Hjelpelag: fil/miljø-host-bridge`
        - `Hjelpelag: samlingshjelparar`
        - `Hjelpelag: teksthjelparar`
        - `Hjelpelag: JSON-parsing/-stringify-lag`
        - `Hjelpelag: diverse convenience-builtins`
    - implikasjon for neste tekniske kutt:
      - den mest naturlege første reduksjonen er ikkje kjernen
      - det er å skilje ut fil/miljø og tekst/JSON som eigne hjelpeflater, fordi dei er tydelegast host-nære og minst sentrale for ein minimal stage0-kjerne
    - konkret delplan for første kutt:
      - A. samle fil/miljø-funksjonane som éi host-bridge-flate
      - B. la generated-C-lanen gå gjennom denne flata i staden for å vere spreidd i mini-runtimeen
      - C. bruk denne grensa til å erstatte eller fjerne heile gruppa samla seinare
    - status no:
      - `archive/legacy_c_backend/nc_runtime_mini.c` har no faktiske `nc_host_*`-wrappere for fil/miljø
      - builtinane for `fil_*`, `miljo_*` og `miljo_sett` går no gjennom denne flata i staden for å kalle `fopen`/`getenv`/`setenv` direkte
      - `nc_host_*` er no også løfta til path-/env-nivå med eigne operasjonar for:
        - les tekst frå path
        - skriv tekst til path
        - skriv binær liste til path
        - append tekst til path
        - eitt samla miljøoppslag som kan gi både kopi og “finnes”-status
        - sett miljøvariabel og slå opp resultat via same samla miljøoppslag
        - felles filfeil-rapportering frå host-flata
        - direkte path-/env-operasjonar frå builtinane, med felles filfeil-flate der det trengst
      - det lågaste mellomlaget for `fopen`/`getenv`/`setenv` er no fjerna; host-flata er mindre og meir fokusert på path-/env-operasjonar
      - tre ein-bruks mellomledd i host-laget er no òg fjerna; les/skriv/binær-skriv er no direkte uttrykt i path-operasjonane
      - fire trivielle `NcVal`-adapterar er no òg fjerna; enkle bool/tekst-tilfelle går direkte frå builtin til path-/env-flata
      - dei siste `NcVal`-wrapperane for `fil_les` / `fil_skriv` / `fil_skriv_binar` / `fil_append` er no òg fjerna; builtinane brukar host-operasjonane direkte mot felles filfeil-flate
      - miljøsida brukar no eitt samla host-oppslag for både verdi og “finnes”-status, i staden for to separate env-entrypoint
      - `miljo_sett` går no òg direkte mot same samla miljøoppslag, utan eige `setenv_and_copy`-mellomledd
      - `miljo_finnes` slepp no unødvendig strengkopi; host-laget har eit lite råoppslag for reine “finnes”-sjekkar
      - lesesida på fil-host-flata deler no òg ein liten felles helper for `tekst eller felles filfeil`
      - skriv-/append-feil på filsida deler no òg ein liten felles host-helper i staden for tre lokale `ok ? nil : feil`-mønster
      - `fil_skriv_binar` følgjer no same host-feilmodell som resten av `fil_*`-familien
      - tekstlaget har no òg eigne `nc_text_*`-helperar for sentrale operasjonar som `starts_with`, `ends_with`, `contains`, `trim`, `lower`, `upper` og `index_of`
      - `split`, `join` og `replace` går no òg gjennom `nc_text_*`-helperar
      - fleire enkle tekst-builtins gjekk først gjennom `NcVal`-vennlege `nc_text_*`-adapterar, men dei mest trivielle bool/tekst/int-wrapperane er no fjerna igjen
      - `split`, `join` og `replace` går no òg direkte frå builtin til rå `nc_text_*`-operasjonar; dei ekstra `NcVal`-adapterane er fjerna
      - dette gjer teksthjelparane til ei meir samla avkoblingsflate, i staden for spreidde direkte kall til `str*`/`ctype`
      - samlingshjelparane har no første eigne `nc_coll_*`-inngangar for:
        - `finnes_nokkel`
        - `nokler`
        - `verdier`
      - JSON-laget har no første eigne `nc_json_*`-inngangar for:
        - whitespace-skip
        - tekst-til-JSON-parse
        - “looks like nonstring”-vurdering
        - samla stringify-inngangar for vanleg og “smart” JSON-stringify
      - den trivielle lokale `json_stringify`-wrapperen er no òg fjerna; interne kall går direkte til `nc_json_stringify_any(v, 0)`
      - den trivielle lokale `json_stringify_smart`-wrapperen er no òg fjerna; interne kall går no direkte til `nc_json_stringify_any(v, 1)`
      - den trivielle lokale parse-dispatch-wrapperen er no òg fjerna; parse-kjernen går direkte til `jp2_parse(...)`
      - `jp2_parse(...)` brukar no òg eigne primitive-entrypointar for:
        - streng
        - `true`
        - `false`
        - `null`
        - heiltal
        - ukjende teikn
      - `true/false/null` vert no parsea via eit token-grense-sjekk i `nc_json_parse_primitive(...)` før verdi-return, for å unngå falske treff som `trueX`/`nulla`
      - desse primitive-entrypointane er samla i eigen `nc_json_parse_primitive(...)` som no er den nye dispatch-innsida frå `jp2_parse(...)`
      - `json_parse_str`, `json_parse_raw` og `json_parse_norscode` deler no éin felles `NcVal -> tekst -> parse`-inngang
      - `json_parse_norscode` har no òg fått bort ei redundant `nil`-vakt etter at den felles parse-inngangen alltid returnerer `NcVal` eller `nc_nil()`
      - `json_parse_norscode` deler no òg éin liten helper for “smart-stringify dersom verdien ikkje alt er tekst”
      - vanleg og smart JSON-stringify deler no òg éin liten liste-helper for sjølve `[` ... `]`-bygginga
      - vanleg og smart JSON-stringify deler no òg éin liten map-helper for sjølve `{` ... `}`-bygginga
      - vanleg JSON-stringify har no òg eige lite escape-helper for tekstverdiar, i staden for å ha escaping inline i entrypointet
      - parse-sida har no òg eige lite unescape-helper for JSON-strengar, i staden for å ha escape-switchen inline i `jp2_str(...)`
      - dette flyttar fleire kallpunkt bort frå spreidd `jp2_*`-bruk og gjer JSON-laget meir samla
    - klare for første større utfasing:
      - `nc_host_*`: ja, dette er den første naturlege større utfasinga
      - `nc_text_*`: ja, som neste samanhengande hjelpeflate etter fil/miljø
      - `nc_json_*`: ja, når fil/miljø-broa er adressert
      - `nc_coll_*`: delvis; samla nok til eiga hjelpeflate, men tettare på kjerna og derfor ikkje første store kutt
  - Restplan no:
    - 1. Fullfør første større utfasing av `nc_host_*`
      - mål: gjere fil/miljø-broa så liten og samla som mogleg
      - ferdig når: `fil_*`/`miljo_*`-builtinane i praksis berre er tynne adapterar mot éi host-flate
      - status no:
        - `fil_les` går no direkte mot path-operasjonen og den same felles filfeil-flata, utan eige eitt-bruks resultatmellomledd
        - `fil_skriv` / `fil_skriv_binar` / `fil_append` går no direkte mot path-operasjonar + liten felles skrive-/feilhelper
        - `miljo_hent` / `miljo_finnes` / `miljo_sett` går no gjennom samla miljøoppslag utan eigne mellomledd for kopiering eller “finnes”-sjekk
        - tekstresultat frå miljøoppslag går no gjennom ein smalare helper `nc_host_getenv_text(...)`, i staden for den meir diffuse `lookup(..., exists)`-forma
        - dette er nær eit godt delmål for første større utfasing; vidare kutt her bør vere reelle forenklingar, ikkje berre flytting av små wrapperar
    - 2. Ta neste større utfasing av `nc_text_*`
      - mål: samle attverande tekstlogikk bak interne tekstoperasjonar
      - ferdig når: tekst-builtins ikkje lenger har spreidd strenglogikk lokalt
      - status no:
        - `starts_with` / `ends_with` / `contains` / `trim` / `lower` / `upper` / `index_of` går no direkte mot små rå `nc_text_*`-operasjonar
        - `split` / `join` / `replace` går no òg direkte mot eigne rå tekstoperasjonar, utan ekstra `NcVal`-wrapperledd
        - tekstlaget er no samla nok til å vere ei eiga hjelpeflate med:
          - reine bool/int-operasjonar
          - streng-til-streng-operasjonar
          - liste/streng-overgangar for `split` og `join`
        - dette er eit godt delmål for `nc_text_*`; vidare arbeid bør vere reell samling av semantikk, ikkje berre nye små adapterar
    - 3. Ta deretter `nc_json_*`
      - mål: gjere parse/stringify-laget til éi samla hjelpeflate
      - ferdig når: JSON-entrypoints ikkje lenger er spreidde mellom fleire lokale hjelpefunksjonar
      - status no:
        - `json_parse_str` / `json_parse_raw` / `json_parse_norscode` deler no felles parse-inngang
        - dei tidlegare tomme C-wrapperane for `json_parse` / `json_parse_raw` er no fjerna; builtin-mappinga peikar direkte på `nc_json_parse_from_ncval(...)`
        - siste reine compat-rest for `json_parse_raw` på C-sida er no òg fjerna
        - parse-kjernen går direkte til `jp2_parse(...)`
        - `jp2_str(...)` brukar no eige lite unescape-helper
        - vanleg og smart stringify deler no:
          - primitive JSON-verdiar (`null`, `bool`, `heltall`) som eiga hjelpeflate
          - strengverdiar som eiga hjelpeflate, med vanleg quoting og smart “looks like nonstring”-åtferd samla eitt stad
          - streng-escape som eiga hjelpeflate
          - top-level stringify-dispatch som eiga hjelpeflate for `primitive -> string -> compound`
          - compound-stringify for `liste` / `map` som eiga hjelpeflate
          - listebygging som eiga hjelpeflate
          - mapbygging som eiga hjelpeflate
        - eitt-bruks mellomleddet for “smart-stringify dersom ikkje tekst” er no òg fjerna; logikken lever no direkte i lagringshelperen som brukar henne
        - `json_parse_norscode` deler no òg små helperar for:
          - “smart-stringify map-verdiar in-place”
          - “konverter liste til string-keyed map med stringifiserte verdiar”
        - dette er no ikkje berre eit godt delmål, men nær eit naturleg stoppunkt for mikrokutt i `nc_json_*`
        - vidare arbeid her bør vere reelle semantiske samlingar eller bevisste arkitekturgrep, ikkje fleire små flyttingar berre fordi dei finst
    - 3b. Vurder `nc_coll_*` som støtteflate tett på kjerna
      - mål: halde samlingshjelparane små, tydelege og utan å late som om dei er like utskiftbare som host/tekst/JSON
      - status no:
        - `finnes_nokkel` går no direkte mot `nc_coll_find_key_index(...)`
        - `nokler` og `verdier` deler no éin liten `nc_coll_*`-helper for listeinnsamling frå map
        - `finnes_nokkel` og `fjern_nokkel` deler no eitt lite nøkkeloppslag i map via `nc_coll_find_key_index(...)`
        - dette er eit godt delmål for `nc_coll_*`: hjelpeflata er tydeleg, men ho ligg framleis nær liste/ordbok-kjerna og bør derfor ikkje drivast like langt som host/tekst/JSON utan god grunn
    - 4. Vurder kva som må bli att som ekte stage0-kjerne
      - mål: skilje resten av `nc_runtime_mini.c` i
        - minimumskjerne som må leve
        - hjelpeflate som kan bort
      - status no:
        - dette ser no tydeleg ut som kandidat for minimal stage0-kjerne i sjølve koden:
          - verdi-layout og grunnstrukturar
          - feilhandtering
          - konstruktørar
          - stack
          - variabeloppslag
          - konvertering/truthiness
          - aritmetikk
          - samanlikning
          - liste/ordbok-bygging
          - indeksering
          - unntak
        - dette står no tydeleg som hjelpeflater med eigne delmål:
          - `nc_host_*`
          - `nc_text_*`
          - `nc_json_*`
          - `nc_coll_*`
          - diverse convenience-builtins
        - førebels minimumsbilete no:
          - må truleg overleve i ei minimal maintainer-bru:
            - `NcVal`-layout og kjerneallokering
            - stack-/lokalvariabelmodell
            - indeks-/map-/listeoperasjonar som generated `nc_fn_*` faktisk brukar
            - grunnleggjande truthiness, samanlikning og aritmetikk
            - unntak/feil som held køyringa saman ved brot
          - kan truleg behandlast som hjelpeflate eller seinare utskiftbar rand:
            - fil/miljø-host-bridge
            - tekstoperasjonar utover kjernekonvertering
            - JSON-parse/-stringify
            - samlingshjelparar for `nokler` / `verdier` / `finnes_nokkel`
            - docs-/OpenAPI-/convenience-builtins
        - tryggleiksnivå i dette minimumsbiletet no:
          - høg tryggleik for kjerne:
            - `nc_push`, `nc_pop`
            - `nc_load`, `nc_store`
            - `nc_nil`, `nc_int`, `nc_bool`, `nc_str`
            - `nc_truthy`, `nc_eq`, `nc_cmp`
            - `nc_add` og resten av heiltals-/bitoperatorane
            - `nc_build_list`, `nc_build_map`
            - `nc_index_get`, `nc_index_set`
            - `nc_throw`
          - middels tryggleik / bør framleis observerast vidare:
            - `nc_map_new` når generatoren brukar map som mellomstruktur
            - grenseflata mellom kjerne og `nc_coll_*`
            - små builtin-hjelparar som `nc_builtin_legg_til` og `nc_builtin_lengde`
            - liten, men viktig gråsone frå generated bruk:
              - `nc_builtin_slice`
              - `nc_builtin_tekst_fra_heltall`
              - `nc_builtin_finnes_nokkel`
              - desse ser framleis ut til å vere såpass mykje brukte at dei bør behandlast som “må observerast vidare”, ikkje som tidlege slettingskandidatar
          - låg tryggleik for å kalle “må leve”:
            - fil/miljø-host-bridge
            - JSON-spesifikke builtins
            - teksthjelparar utover kjernekonvertering
        - tidleg verifisert direkte frå `archive/legacy_c_backend/ncb_to_c.no`:
          - kodegeneratoren emitterer direkte kall til denne kjerneflata:
            - `nc_push`, `nc_pop`
            - `nc_store`, `nc_load`
            - `nc_truthy`
            - `nc_add`, `nc_sub`, `nc_mul`, `nc_div`, `nc_mod`
            - `nc_lshift`, `nc_rshift`, `nc_band`, `nc_bor`, `nc_bxor`, `nc_neg`
            - `nc_eq`, `nc_cmp`
            - `nc_build_list`, `nc_build_map`
            - `nc_index_get`, `nc_index_set`
            - `nc_nil`, `nc_int`, `nc_bool`, `nc_str`
            - `nc_map_new`
            - `nc_throw`
          - dette styrkjer vurderinga av kva som faktisk er minimal stage0-kjerne, fordi desse namna kjem frå sjølve opcode→C-emitteringa og ikkje berre frå helper-lag rundt
        - observert frå faktisk generated maintainer-output (`bootstrap/maint/c/norscode_generated.c` og `build/regen_verify_{a,b}/maint/c/norscode_generated.c`):
          - svært tunge treff ligg nettopp i kjerneflata:
            - `nc_push`, `nc_pop`
            - `nc_load`, `nc_store`
            - `nc_str`, `nc_int`, `nc_bool`, `nc_nil`
            - `nc_truthy`, `nc_eq`, `nc_cmp`
            - `nc_add`
            - `nc_index_get`, `nc_index_set`
            - `nc_build_list`, `nc_build_map`
          - hjelpeflate-/builtin-treff finst, men ligg langt lågare og meir spreidd:
            - `nc_builtin_legg_til`, `nc_builtin_lengde`
            - `nc_builtin_miljo_hent`, `nc_builtin_fil_les`
            - `nc_builtin_json_stringify`
            - `nc_builtin_slice`, `nc_builtin_starts_with`
          - dette støttar at kjernelista i punkt 4 ikkje berre er strukturmessig rimeleg, men også treff faktisk bruksmønster i generated output
        - førebels bevar-liste for alternativ A:
          - desse bør i praksis behandlast som “rør ikkje først” i maintainer-brua:
            - verdiar og grunnkonstruktørar:
              - `nc_nil`, `nc_int`, `nc_bool`, `nc_str`
            - stack og lokalvariablar:
              - `nc_push`, `nc_pop`, `nc_load`, `nc_store`
            - grunnleggjande evaluering:
              - `nc_truthy`, `nc_eq`, `nc_cmp`
            - aritmetikk/bitoperatorar:
              - `nc_add`, `nc_sub`, `nc_mul`, `nc_div`, `nc_mod`
              - `nc_lshift`, `nc_rshift`, `nc_band`, `nc_bor`, `nc_bxor`, `nc_neg`
            - container-/indeks-kjerne:
              - `nc_build_list`, `nc_build_map`, `nc_index_get`, `nc_index_set`
              - `nc_map_new` når generatoren treng map som mellomstruktur
            - feilhald:
              - `nc_throw`
          - grunngiving:
            - desse er både direkte emitterte frå `ncb_to_c.no`
            - og blant dei tyngst brukte `nc_*`-kalla i generated maintainer-output
          - praktisk konsekvens:
            - vidare alternativ-A-kutt bør skje rundt hjelpeflater og randbuiltins
            - ikkje inne i denne lista utan ny, konkret brukskontroll
        - førebels minimumskontrakt for alternativ A:
          - desse gruppene bør no behandlast som ei praktisk bevaringskontrakt for at maintainer-brua framleis skal vere truverdig:
            - verdi-/objektkjerne:
              - `nc_nil`, `nc_int`, `nc_bool`, `nc_str`
            - stack-/lokalvariabelkontrakt:
              - `nc_push`, `nc_pop`, `nc_load`, `nc_store`
            - evalueringskontrakt:
              - `nc_truthy`, `nc_eq`, `nc_cmp`
            - aritmetikk-/bitkontrakt:
              - `nc_add`, `nc_sub`, `nc_mul`, `nc_div`, `nc_mod`
              - `nc_lshift`, `nc_rshift`, `nc_band`, `nc_bor`, `nc_bxor`, `nc_neg`
            - container-/indekskontrakt:
              - `nc_build_list`, `nc_build_map`, `nc_index_get`, `nc_index_set`
            - throw-/feilkontrakt slik generatoren emitterer henne i dag:
              - `nc_throw`
              - `nc_to_str`
            - strukturkonstruktør-kontrakt så lenge generatoren framleis emitterer tom map for desse:
              - `nc_map_new`
          - denne kontrakten er ikkje berre strukturmessig:
            - ho er støtta både av direkte emittering i `ncb_to_c.no`
            - og av tung faktisk bruk i generated maintainer-output
            - for `nc_map_new` er støtta litt annleis:
              - han er ikkje blant dei tyngst brukte kjernesymbola
              - men han er framleis direkte emittert frå `ncb_to_c.no` for struktur-konstruktørar med stor forbokstav
              - og kjem derfor framleis frå generator-kjernen, ikkje berre frå randflate/helper-lag
            - for `nc_throw` er støtta òg litt særskild:
              - generatoren emitterer i dag fast mønsteret `nc_throw(nc_to_str(e))` for `THROW`
              - det gjer sjølve throw-vegen meir generatorbunden enn mange vanlege helper-kall
              - og betyr at både `nc_throw` og `nc_to_str` bør lesast som del av same aktive kontrakt så lenge emitteringa er slik
            - for `nc_to_str_raw` er biletet annleis:
              - han er mykje breiare brukt som intern teksthelper i aktiv runtimefil
              - han høyrer derfor ikkje til same smale generatorbundne kontrakt som `nc_throw(nc_to_str(e))`
              - han bør heller lesast som intern hjelpeflate som delvis støttar fleire lag, også utanfor sjølve throw-vegen
            - for `nc_builtin_feil` er biletet mellom desse:
              - han har framleis reell generated bruk via builtin-mapping i `ncb_to_c.no`
              - men han er ikkje sjølve generatorens `THROW`-kontrakt
              - han bør derfor lesast som randnær convenience-builtin som gjenbruker throw-/teksthelperane, ikkje som del av den smale `THROW`-emitteringa
          - endringar som rører denne kontrakten bør derfor krevje ny verifisering før dei blir rekna som trygge:
            - ny sjekk mot direkte emitterte `nc_*`-kall frå `ncb_to_c.no`
            - ny sjekk mot faktisk generated maintainer-output / verify-output
            - ny vurdering av om endringa berre flyttar randlogikk, eller faktisk endrar stage0-kjerneoppførsel
          - praktisk arbeidsregel:
            - om eit kutt kan gjerast utan å endre denne kontrakten, høyrer det framleis heime i alternativ-A-randflata
            - om eit kutt krev endring i denne kontrakten, er det ikkje lenger eit vanleg randkutt, men eit punkt-4- eller punkt-5-grep
        - må re-verifiserast dersom rørt:
          - dette bør no behandlast som tydelege stopp-/kontrollpunkt i alternativ A:
            - endring i signatur, returtype eller grunnoppførsel for:
              - `nc_push`, `nc_pop`, `nc_load`, `nc_store`
              - `nc_truthy`, `nc_eq`, `nc_cmp`
              - `nc_add`, `nc_sub`, `nc_mul`, `nc_div`, `nc_mod`
              - `nc_lshift`, `nc_rshift`, `nc_band`, `nc_bor`, `nc_bxor`, `nc_neg`
              - `nc_build_list`, `nc_build_map`, `nc_index_get`, `nc_index_set`
              - `nc_nil`, `nc_int`, `nc_bool`, `nc_str`
            - endring i generatorforventninga rundt struktur-konstruktørar:
              - `nc_map_new` så lenge `ncb_to_c.no` framleis emitterer tom map for slike kall
            - endring i generatorforventninga rundt `THROW`:
              - `nc_throw`
              - `nc_to_str`
              - særleg dersom `ncb_to_c.no` ikkje lenger emitterer `nc_throw(nc_to_str(e))` som fast mønster
            - endring i `nc_to_str_raw(...)` bør ikkje automatisk lesast på same måte:
              - han er framleis viktig
              - men han er ikkje like eintydig generatorbunden som `nc_to_str(...)`
              - inngrep der bør vurderast som brei helper-/runtimeendring, ikkje berre som THROW-kontrakt
            - endring i semantikken for stackbruk:
              - push/pop-rekkjefølgje
              - kven som eig returverdiar
              - korleis locals blir lagra og lasta
            - endring i semantikken for containerkjerne:
              - korleis liste/map blir bygde frå stacken
              - korleis indexing les eller skriv verdiar
            - endring som flyttar noko frå randflate inn i kjernelaget, eller omvendt, utan ny brukskontroll
          - minimumsverifisering etter slike inngrep bør vere:
            - sjekk mot direkte emittering i `ncb_to_c.no`
            - sjekk mot generated maintainer-output / verify-output
            - eksplisitt vurdering av om generated `nc_fn_*` no forventar noko anna enn før
        - kjernesjekkliste før og etter endring:
          - før endring:
            - avklar om endringa rører minimumskontrakten eller berre randflata
            - sjekk om ho treffer kjernesymbol, gråsone-builtins eller berre randhelperar
            - avgjer om dette framleis er eit alternativ-A-kutt, eller eigentleg eit punkt-4-/punkt-5-grep
          - etter endring:
            - sjekk at direkte emitterte `nc_*`-kall i `ncb_to_c.no` framleis har ein gyldig runtimeflate
            - sjekk særskilt om struktur-konstruktørar framleis blir emitterte som `nc_map_new()` eller om generatorforventninga faktisk er endra
            - sjekk særskilt om `THROW` framleis blir emittert som `nc_throw(nc_to_str(e))`, eller om throw-kontrakten faktisk er endra
            - sjekk at generated maintainer-output / verify-output ikkje no peikar på broten eller flytta kjernekontrakt
            - sjekk at endringa ikkje flytta semantikk frå randflate inn i kjernelaget utan at det var tilsikta
          - stoppsignal:
            - om du må forklare endringa som “berre liten” medan ho faktisk endrar stack, indexering, containerbygging eller feilkontrakt, skal ho behandlast som kjernearbeid
            - om generated bruk framstår meir sentral etter endringa enn før, skal randkutt-sporet stoggast og punkt 4 lesast på nytt
        - førebels randflate som kan krympast først i alternativ A:
          - låg-risiko rand:
            - docs-/OpenAPI-/stubbar som ikkje lenger har aktiv mapping
            - stale generated-restar
            - openbert redundant compat-lag
          - aktiv, men framleis randnær hjelpeflate:
            - `nc_host_*` rundt `fil_*` og `miljo_*`
            - `nc_json_*` rundt `json_stringify` / `json_parse_norscode`
            - `nc_text_*` for `starts_with`, `ends_with`, `contains`, `trim`, `join`, `split`, `replace`, `index_of`, `lower`, `upper`
          - randbuiltins med låg, men reell bruk som framleis bør sjåast som kandidatar før kjernelista:
            - `nc_builtin_skriv`
            - `nc_builtin_index_of`
            - `nc_builtin_lower`
            - `nc_builtin_upper`
            - `nc_builtin_type`
            - `nc_builtin_bool`
            - `nc_builtin_feil`
            - `nc_builtin_n`
            - `nc_builtin_desimaltall`
          - konkret lågrisikokutt som alt er teke i denne randflata:
            - éin-bruks host-mellomledd `nc_host_throw_file_error(...)` er fjerna
            - filfeil går no direkte gjennom `nc_host_file_error_nil(...)`
            - éin-bruks JSON-mellomledd `nc_json_parse_text_raw(...)` er fjerna
            - `nc_json_parse_from_ncval(...)` går no direkte til `jp2_parse(...)`
            - tom JSON-videresendar `nc_json_stringify_smart_value(...)` er fjerna
            - smart-stringify går no direkte gjennom `nc_json_stringify_any(v, 1)`
            - éin-bruks tekstmellomledd `nc_text_index_of(...)` er fjerna
            - `nc_builtin_index_of(...)` brukar no den samla rå helperen `nc_text_index_of_raw(...)`
            - éin-bruks tekstmellomledd `nc_text_contains(...)` er fjerna
            - `nc_builtin_contains(...)` brukar no den samla rå helperen `nc_text_contains_raw(...)`
            - éin-bruks tekstmellomledd `nc_text_ends_with(...)` er fjerna
            - `nc_builtin_ends_with(...)` brukar no den samla rå helperen `nc_text_ends_with_raw(...)`
            - éin-bruks tekstmellomledd `nc_text_starts_with(...)` er fjerna
            - `nc_builtin_starts_with(...)` brukar no den samla rå helperen `nc_text_starts_with_raw(...)`
            - éin-bruks tekstmellomledd `nc_text_lower_copy(...)` er fjerna
            - `nc_builtin_lower(...)` gjer no lowercase-konverteringa direkte
            - éin-bruks tekstmellomledd `nc_text_upper_copy(...)` er fjerna
            - `nc_builtin_upper(...)` gjer no uppercase-konverteringa direkte
            - éin-bruks tekstmellomledd `nc_text_trim_copy(...)` er fjerna
            - `nc_builtin_trim(...)` gjer no trim-operasjonen direkte
          - delmål no:
            - randflata er no tydeleg krympa utan å gå inn i gråsona nær kjernen
            - vidare mikrokutt herfrå vil truleg gi mindre avkastning per endring enn det serien alt har gitt
            - neste arbeid bør derfor som hovudregel gå tilbake til større punkt-4-vurderingar eller meir samanhengande helper-kutt, ikkje fleire einskilde eitt-bruks fjerningar berre fordi dei finst
          - viktig avgrensing:
            - `nc_builtin_legg_til`, `nc_builtin_lengde`, `nc_builtin_slice`, `nc_builtin_tekst_fra_heltall` og `nc_builtin_finnes_nokkel` høyrer no til gråsona nær kjernen, ikkje denne randlista
          - verifiseringsnyanse:
            - sjekk-innlagde/generated artefaktar kan vere eldre enn aktiv `nc_runtime_mini.c`
            - til dømes kan `bootstrap/maint/c/norscode_generated.c` framleis innehalde ein stub som alt er fjerna frå kjeldekoden
            - når dette skjer, skal punkt 4 stole meir på aktiv kjelde + nyare verify-output enn på eitt enkelt eldre generated snapshot
        - trygge alternativ-A-kutt no:
          - dette kan framleis reknast som vanlege randkutt så lenge minimumskontrakten ikkje blir rørt:
            - fjerne eitt-bruks helperar i `nc_host_*`, `nc_text_*` eller `nc_json_*`
            - fjerne tomme videresendarar eller openbert redundante compat-ledd
            - fjerne ubrukte stubbar eller stale generated-restar som ikkje lenger har aktiv mapping
            - flytte lokal randlogikk til eit nærliggande aktivt kallpunkt utan å endre semantikken som generated `nc_fn_*` ser
          - dette bør ikkje lenger reknast som “trygt randkutt”, sjølv om det ser lite ut:
            - endring i kven som eig verdiar på stacken
            - endring i returverdi eller feiloppførsel for kjernesymbol
            - endring i korleis liste/map/index-kallet faktisk verkar
            - endring i gråsone-builtins utan ny brukskontroll
            - endring i `nc_builtin_feil(...)` utan å kontrollere både builtin-mapping og generated bruk
          - tommelfingerregel:
            - om endringa berre gjer randflata flatere, er ho framleis i alternativ A
            - om endringa gjer generated-kjernekontrakten annleis, har ho gått ut av randflata
        - ikkje verd å jage no:
          - dette bør no som hovudregel ikkje vere mål for nye mikrokutt utan ny, særskild grunn:
            - fleire einskilde eitt-bruks helper-kutt i randflata berre fordi dei framleis finst
            - generell sletting av convenience-builtins som framleis har reell generated bruk
            - omskriving av gråsone-builtins utan ny brukskontroll
            - nye kommentar- eller namneoppryddingar i aktiv runtimefil som ikkje endrar faktisk overflate
            - kandidatar som ser små ut i aktiv kjelde, men som framleis er direkte brukte i generated output, til dømes:
              - `nc_to_str(...)` i throw-emitteringa
              - `nc_builtin_bool(...)`
              - `nc_builtin_n(...)`
              - små `nc_fn_builtin_*`-compat-wrapperar som eldre generated output framleis peikar på
            - samstundes:
              - `nc_to_str_raw(...)` ser liten ut frå utsida, men er breitt brukt interntekst-helper i runtimefila og er derfor heller ikkje noko godt mikrokutt
              - `nc_builtin_feil(...)` ser enkel ut, men har framleis reell generated bruk og ligg på grensa mellom convenience-lag og throw-gjenbruk
          - grunngiving:
            - randflata er allereie tydeleg krympa
            - planen har no både minimumskontrakt, randflate og re-verifiseringsreglar
            - vidare gevinst kjem truleg meir frå betre kjerneskille og bevisste arkitekturgrep enn frå fleire små lokale oppryddingar
        - “kan bort først” for alternativ A no:
          - lågast risiko:
            - docs-/OpenAPI-/convenience-builtins
            - fil/miljø-host-bridge
          - deretter:
            - JSON-spesifikke builtins og JSON-hjelpelag
            - teksthjelparar utover kjernekonvertering
          - seinare og med meir varsemd:
            - `nc_coll_*`
            - små builtin-hjelparar som `nc_builtin_legg_til` og `nc_builtin_lengde`
            - alt som grensar tett mot liste/map/index-kjerna
          - bør ikkje vere først:
            - stack-/variabel-/verdilayout
            - samanlikning/aritmetikk/truthiness
            - indeks-/liste-/map-kjerne
          - første konkrete lågrisikokutt er no allereie teke:
            - ubrukt OpenAPI-stub `nc_builtin_openapi_json(...)` er fjerna frå `nc_runtime_mini.c`
          - viktig nyanse etter ny generated-gjennomgang:
            - dette er ei prioritert randliste, ikkje ei automatisk sletteliste
            - fleire convenience-builtins er framleis faktisk i bruk i generated maintainer-output, sjølv om dei ligg langt lågare enn kjernelaga
            - observerte døme frå `bootstrap/maint/c/norscode_generated.c`:
              - `nc_builtin_skriv` ~36 treff
              - `nc_builtin_slice` ~46 treff
              - `nc_builtin_tekst_fra_heltall` ~46 treff
              - `nc_builtin_finnes_nokkel` ~20 treff
              - `nc_builtin_index_of` ~7 treff
              - `nc_builtin_lower` / `nc_builtin_upper` / `nc_builtin_type` / `nc_builtin_bool` / `nc_builtin_feil` / `nc_builtin_n` / `nc_builtin_desimaltall` med låge, men reelle treff
            - dette styrkjer at lågrisiko-kutt først bør vere:
              - ubrukte stubbar
              - openbert redundant compat-lag
              - eller stale generated-restar
            - og ikkje generell sletting av convenience-builtins utan ny brukskontroll
            - same gjeld små `nc_fn_builtin_*`-compat-restar:
              - `nc_fn_builtin_tekst_erstatt(...)`
              - `nc_fn_builtin_tekst_starter_med(...)`
              - dei ser randaktige ut i aktiv kjelde, men finst framleis i generated maintainer-output og må difor lesast som compat-lag, ikkje som daud kode
            - verifisert biletet no:
              - aktiv `nc_runtime_mini.c` har berre desse `nc_fn_builtin_*`-restane:
                - host-/generated hook: `nc_fn_builtin_neste_token(...)`
                - compat-wrapperar: `nc_fn_builtin_tekst_erstatt(...)`, `nc_fn_builtin_tekst_starter_med(...)`
              - eldre generated output kan framleis vise fleire namn, til dømes `nc_fn_builtin_json_parse_raw(...)`, utan at dei lenger finst i aktiv kjelde
              - dette betyr at ikkje alle `nc_fn_builtin_*`-treff i gamle generated artefaktar skal tolkast som aktiv runtime-overflate
              - same lesemåte gjeld små historiske kommentarrestar: når aktiv kjelde alt er rydda, skal ikkje gamle generated markørar eller eldre kommentarspor få styre vurderinga av aktiv overflate
              - målet i aktiv runtimefil er derfor no nøkterne kommentarar som forklarer aktiv rolle, ikkje eldre migrasjonshistorikk
              - dette gjeld også filhovud og hjelpeflate-seksjonar: dei bør beskrive aktiv rolle i maintainer-lanen, ikkje tidlegare oppryddingsstatus
              - konkret følgd opp no:
                - status-/delplan-kommentarar i `nc_runtime_mini.c` er stramma inn til aktive seksjonsbeskrivingar for host-, tekst- og JSON-laga
                - små implementasjonskommentarar er òg gjort meir lokale og nøkterne, til dømes rundt strenglengd-cache, map-vekst og JSON-bufferbruk
                - rangordningsspråk som “sist ut” og liknande er fjerna frå aktiv runtimefil der det berre uttrykte oppryddingsrekkefølgje
            - praktisk lesehjelp:
              - `nc_builtin_legg_til`, `nc_builtin_lengde`, `nc_builtin_slice`, `nc_builtin_tekst_fra_heltall` og `nc_builtin_finnes_nokkel` bør no lesast som “gråsone nær kjernen”
              - `nc_builtin_index_of`, `nc_builtin_lower`, `nc_builtin_upper`, `nc_builtin_type`, `nc_builtin_bool`, `nc_builtin_feil`, `nc_builtin_n` og `nc_builtin_desimaltall` ligg framleis i randflata, men er ikkje døde
        - konkret verifiseringsrekkjefølgje for dette minimumsbiletet:
          - bruk faktisk maintainer-output eller generert output, ikkje ein antatt fast filplassering
          - start med `bootstrap/kompiler.ncb.json` som stabil referanse for kva funksjonsmengda faktisk er
          - bruk maintainer-generert `norscode_generated.c` når slikt output finst, i staden for å anta at `archive/legacy_c_backend/norscode_generated.c` eksisterer
          - samanlikn observerte `nc_*`-kall frå generated `nc_fn_*` mot lista over førebels minimumskjerne
          - juster først deretter kva som faktisk må stå att som kjerne
        - det betyr at punkt 4 ikkje lenger er hypotetisk: skiljet finst no både i planen og i kodekommentarane i `nc_runtime_mini.c`
        - status no:
          - punkt 4 er no skarpt nok til å fungere som faktisk styringsgrunnlag for alternativ A
          - vidare arbeid under punkt 4 bør no bruke dette skiljet aktivt, ikkje finjustere det vidare med fleire små presiseringar
        - neste arbeid under punkt 4 bør derfor vere meir samanhengande helper-/kjernegrep, ikkje meir generell kartlegging
    - 5. Ta arkitekturvalet for siste C-bro
      - Alternativ A: hald generated-C-brua som smal maintainer-nødbane.
        Dette betyr å behalde `ncb_to_c.no`, `nc_runtime_mini.c` og `nc_native_main.c`, men halde dei tydeleg avgrensa til `tools/maint/*` og seed-fornying.
      - Alternativ B: erstatt generated-C-brua med ei ny maintainer-løype utan generated C.
        Dette krev ny seed-/rebuild-kontrakt, ny handtering av host-FFI og ein annan måte å produsere køyrbar maintainer-seed på enn `clang` + `norscode_generated.c`.
      - Førebels retning:
        Vel alternativ A på kort sikt. Det held prosjektet stabilt medan vi fullfører skiljet mellom stage0-kjerne og hjelpeflater, og gjer at eit eventuelt alternativ B kan takast som eit eige, bevisst steg seinare.
      - Arbeidsregel:
        I mellomtida skal generated-C-brua berre få reelle forenklingar, ikkje nye utvidingar. Endringar som rører kjernekontrakten skal behandlast som eigne arkitekturgrep, ikkje som vanleg opprydding.
      - Praktisk neste steg:
        Dersom vi jobbar vidare innanfor Fase 3, er det mest verdifulle å ta større, samanhengande alternativ-A-grep i helperflata, ikkje fleire små mikrokutt eller eit halvstarta alternativ-B-skifte.
    - Avklart no:
      - Skiljet mellom stage0-kjerne og hjelpeflater er no tydeleg nok til å styre vidare arbeid.
      - `nc_host_*`, `nc_file_*`, `nc_env_*`, `nc_path_*`, `nc_text_*` og `nc_json_*` er samla nok til å fungere som eigne helperflater rundt maintainer-kjernen.
      - `nc_coll_*` er ei lita støtteflate tettare på kjernen og er derfor ikkje prioritert for aggressiv vidare utfasing.
      - Vidare arbeid i alternativ A bør vere samanhengande forenklingar i desse helperflatene, ikkje fleire små adapterkutt eller ny generell kartlegging.

Verifikasjon:

```bash
rg --files | rg '(^|/).*\.(c|h)$'
rg -n 'clang|gcc|cc|nc_native_main|nc_runtime|bootstrap/maint/c' .github tools selfhost docs bin bootstrap
```

Ferdig naar:

- [x] Ingen `.c` eller `.h` finnes utenfor eventuell `archive/`.
- [x] Ingen normal kommando omtaler eller bruker C. (Vedlikehaldskommandoar i `bin/nc` er no eksplisitt merka `[maintainer]`, inkludert `regen-native`, `regen-verify`, `verify-l6`, `ensure-stage0-seed` og `finish-6b4`.)
- [x] Maintainer-verifikasjonane for attverande C-brua passerer isolert:
  - `bash tools/verify_nc_main_host.sh`
  - `bash tools/maint/regen_verify.sh`
  - `bash tools/maint/verify_l6.sh`

Status no:

- Fase 3 og Fase 4 er gjennomført for normalvegen; attverande arbeid er vedlikehaldsfokus.
- Fase 3-resten er eksplisitt definert som vedlikehald: `archive/legacy_c_backend/*` + `tools/maint/*`-banar.
- Fase 3 er no òg verifisert for den attverande maintainer-brua via `verify_nc_main_host`, `regen_verify` og `verify_l6`.
- Full normalflate er òg re-verifisert grønn via `bash tools/verify_selvstendighet.sh` etter JSON-kompatfiksen i `std.json`.
- Attverande vedlikehaldsoppgåve er avgrensa til to ting:
  - halde maintainer-workflowar og `tools/maint/*` tydeleg merka som unntak frå normalvegen
  - redusere direkte generated-C-avhengigheit vidare berre dersom det gir reell forenkling av maintainer-brua
- Neste bevisste val er ikkje om normalvegen er ferdig, men om maintainer-brua skal:
  - bevarast som smal nødbane
  - eller erstattast av ein annan seed-/maintainer-mekanisme seinare

## Fase 4: Avvikle `ncb_to_c`

Maal: C-backend er ikke lenger et operativt spor.

- [x] Bekreft at `archive/legacy_c_backend/ncb_to_c.no` kun brukes av vedlikeholdsbanen (L4/L6) og ikkje av normal CI/use-case.
  (Tilsyn via `tools/no_c_python_active_surface.sh` allowlist for `ncb_to_c`.)
- [x] Flytt `ncb_to_c.no` til `archive/legacy_c_backend/` eller slett den når `ncb_to_c`-banen er fullstendig avvikla.
- [x] Flytt dokumentasjonen bort frå `ncb_to_c` som normal anbefalt vei.
- [x] Legg CI-gate som feilar ved nye `ncb_to_c`-referansar i aktiv normal-surface.

Verifikasjon:

```bash
rg -n 'ncb_to_c' .github tools selfhost docs bin bootstrap
```

Ferdig naar:

- [x] `ncb_to_c` finnes ikke i aktiv kildeflate.
- [x] CI feiler hvis ny normalflyt gjeninnforer C-backend.

## Fase 5: Fjern Python

Maal: ingen Python i aktiv kildeflate eller CI.

- [x] Erstatt `tools/python_dependency_audit.sh` med `.no`-basert audit.
- [x] Erstatt `tools/python_free_ci.sh` med `.no`-basert gate eller ren shell-gate uten Python-avhengighet.
- [x] Erstatt Omgang 6b-fragmentregenerering i `tools/` med `.no`-verktøy.
  - `tools/regenerate_omgang6b_fragments.py` er fjerna.
  - `selfhost/tooling/regenerate_omgang6b_fragments.no` regenererer no `bootstrap/precompiled_fragments*`.
- [x] Fjern `reports/python_dependency_report.txt` eller erstatt med Norscode-generert rapport.
- [x] Finn og slett eventuelle gjenværende `.py`-filer.
- [x] Fjern Python-omtale fra normal docs, bortsett fra historiske notater.
- [x] Legg CI-gate som feiler ved `.py` utenfor arkiv.

Verifikasjon:

```bash
rg --files | rg '\.py$'
rg -n 'python|python3|pytest|\.py' .github tools selfhost docs bin bootstrap
```

Ferdig naar:

- [x] Ingen `.py` finnes utenfor eventuell `archive/`.
- [x] Ingen CI-steg bruker Python.

## Fase 6: Rydd arkiv og dokumentasjon

Maal: nye bidragsytere ser bare en sann normalvei.

- [x] Oppdater [SELFHOST_HANDLINGSPLAN.md](SELFHOST_HANDLINGSPLAN.md).
- [x] Oppdater [SELVSTENDIGHET_PLAN.md](SELVSTENDIGHET_PLAN.md).
- [x] Oppdater [START_HER.md](START_HER.md).
- [x] Oppdater [CLI_CONTRACT.md](CLI_CONTRACT.md).
- [x] Oppdater [ARCHIVE_INDEX.md](ARCHIVE_INDEX.md).
- [x] Fjern foreldede referanser til C/Python som operativ vei.
- [x] Merk eventuell gjenværende historikk som `archive only`.

Verifikasjon:

```bash
rg -n 'Python|python|C-VM|c_minimal_vm|clang|gcc|ncb_to_c|nc_runtime|nc_native_main' docs
```

Ferdig naar:

- [x] Dokumentasjonen beskriver `.no -> NCB JSON -> selfhost/vm.no/native ELF` som eneste normale vei.
- [x] Historikk er tydelig skilt fra aktive kommandoer.

## Fase 7: Hard CI-sperre

Maal: C/Python kan ikke snike seg tilbake.

- [x] Lag `tools/no_c_python_active_surface.sh`.
- [x] Gate feiler ved `.c`, `.h`, `.py` utenfor allowlist.
- [x] Gate feiler ved `python`, `python3`, `pytest`, `clang`, `gcc`, `cc`, `ncb_to_c`, `c_minimal_vm` i aktiv flyt.
- [x] Allowlist er tom eller begrenset til `archive/`.
- [x] Legg gaten i CI før dyre tester.
- [x] Fjerne `clang`-installasjon fra `stage0-binary.yml` og `selfhost-lexer-token-smoke.yml`.
- [x] Gjør vedlikeholds-workflow-unntak eksplisitte i `tools/no_c_python_active_surface.sh`.
- [x] Legg vedlikeholdsworkflow-forløp (`regen_bootstrap.yml`, `export-stage0-linux.yml`) som eksplisitt vedlikeholdsmodus i plan/ docs, og unntatt fra normal seed-first policy.
- [x] Dokumenter vedlikeholdsbaner som tillatt unntak i `no_c_python_active_surface.sh` (klar policytekst i planen).

Notat: vedlikeholdsmodus er no eksplisitt definert i `tools/no_c_python_active_surface.sh` med
klare policylinjer som krev at unntaka skjer i `regen_bootstrap.yml` og `export-stage0-linux.yml`
og ikkje i normal kjede. Policyteksten seier no også eksplisitt at vedlikehaldsmodus brukar
isolert output som standard, ikkje repoets aktive flate.

Verifikasjon:

```bash
bash tools/no_c_python_active_surface.sh
gh pr checks
```

Ferdig naar:

- [x] Alle CI-checks er grønne.
- [x] En test-PR med `.py` eller `.c` i aktiv flate feiler gaten.
  - Verifisert lokalt med midlertidige `active_gate_probe.py` og `active_gate_probe.c`: begge gir exit code 1, medan rein gate gir exit code 0.

## Endelig akseptanse

- [x] `rg --files | rg '(\.c$|\.h$|\.py$)'` gir ingen aktive filer.
- [x] Normalflaten er fri for C/Python-aktive overflatereferansar.
  - Verifisert med `bash tools/no_c_python_active_surface.sh`.
  - Historiske og vedlikeholdsreferansar er lovlege i `archive/`, vedlikehalds-workflows og merkja plan/status-dokumentasjon.
- [x] `bash tools/verify_selvstendighet.sh` passerer.
- [x] `./bin/nc check` passerer.
- [x] `./bin/nc test` passerer.
- [x] `./bin/nc verify-omgang6b` passerer.
  - På Darwin/arm64 passerer host/NCB/ELF-determinisme og 6b.3 Gen1-bygg; djup ELF-køyring er framleis Linux x86_64-spesifikk.
- [x] Release kan bygges fra stage-0 seed og `.no` uten C/Python.
  - Verifisert lokalt med `bash tools/verify_seed_only.sh`, `./bin/nc selfcheck` og `bash tools/build_stage0_release_assets.sh`; normal `publish.yml` bruker seed → `build_norscode_native.sh` utan C/Python i release-jobbane.

## Reststatus

- [x] Native ELF-runtime, stage-0 seed og Linux x86_64 Gen1 ELF-bundle er verifiserte nok til normal utvikling, test og release.
- [x] Historikk og vedlikehaldsunntak er skilde frå normalflata i både dokumentasjon og CI-gater.
- [x] Attverande C-bruk er no ein eksplisitt maintainer-bru, ikkje ein operativ risiko i normalvegen.
- [ ] Neste eventuelle reduksjon er eit bevisst vedlikehalds-/arkitekturval, ikkje ein blocker for sjølvstendig normalflyt.

## Kort regel

Hvis en normal kommando trenger C eller Python, er planen ikke ferdig.

Hvis C eller Python bare finnes som tydelig arkivert historikk, og CI hindrer at det blir aktivt igjen, er vi ferdige.
