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
- [ ] Verifiser Gen1 ELF -> Gen2 NCB -> Gen2 ELF uten host-kopi.
  - [ ] Linux x86_64 sjølvkompilering står att som eigen ELF/stage-0-verifikasjon; resten av L1-L6-kjeda er no grønn lokalt.
  - [ ] Linux CI køyrer denne med `NC_OM6B_RUN_STAGE0=1` og prøver no ekte preset-bundle først; ved Gen1 ELF-krasj prøver han direkte source-NCB, og berre deretter transitional chunked source-NCB, utan shell-kopi.
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
- [x] Dokumenter runtime-adresse-/ABI-kontrakt for `native_codegen_v2.no` i `docs/NATIVE_CODEGEN_V2_ABI.md`.

Verifikasjon:

```bash
NC_OM6B_RUN_STAGE0=1 bash tools/selfcompile_stage0_elf.sh
./bin/nc verify-omgang6b
```

Ferdig naar:

- [ ] `ELF self-compile paritet (Linux x86_64)` passerer med ekte Gen1 ELF bundle.
- [ ] Ingen paritetstest er avhengig av shell-kopi av NCB som erstatning for ELF-kjoring.

## Fase 2: Gjør stage-0 til binærkontrakt

Maal: stage-0 er en verifisert Norscode-binær, ikke en C-byggvei.

- [x] Lag manifest for `bootstrap/stage0/norscode-*` med SHA256.
- [x] Gjør `tools/build_norscode_native.sh` seed-only i normalmodus.
- [x] Fjern `NORSCODE_BOOTSTRAP_C=1` fra normal buildscript.
- [x] Fjern clang/cc fallback frå normal buildscript.
- [x] Legg inn eksplisitt feilmelding hvis seed mangler.
- [x] Dokumenter hvordan ny stage-0 seed produseres fra Norscode-native pipeline.
- [x] Bruk seed-only i `format-lint-check` i CI.
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

## Fase 3: Fjern C-host og C-runtime

Maal: ingen aktiv C-kilde trengs i repoet.

- [x] Flytt funksjonalitet frå `tools/maint/c/nc_native_main.c` til `.no`.
- [ ] Flytt runtime-kontrakt frå `tools/maint/c/nc_runtime_full.c` til `.no`/native emitter. (Delvis: filen er flytta ut; full kontrakt-evaluering står att.)
- [x] Fjern `tools/maint/c/nc_runtime_mini.c` frå `tools/maint/c/`.
- [x] Fjern eller arkiver hele `tools/maint/c/`.
- [x] Fjern alle referanser til `bootstrap/maint/c/` i normal docs og scripts. (Attverande referansar er avgrensa til vedlikehaldsskript, vedlikehalds-workflowar og historikk.)
- [ ] Fjern C-genererte artefakter fra build/release-flow.

Verifikasjon:

```bash
rg --files | rg '(^|/).*\.(c|h)$'
rg -n 'clang|gcc|cc|nc_native_main|nc_runtime|bootstrap/maint/c' .github tools selfhost docs bin bootstrap
```

Ferdig naar:

- [x] Ingen `.c` eller `.h` finnes utenfor eventuell `archive/`.
- [x] Ingen normal kommando omtaler eller bruker C. (Vedlikehaldskommandoar i `bin/nc` er no eksplisitt merka `[maintainer]`.)

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
og ikkje i normal kjede.

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
- [ ] `rg -n 'python3?|pytest|clang|gcc|ncb_to_c|c_minimal_vm|nc_runtime|nc_native_main' .github tools selfhost docs bin bootstrap` gir ingen aktive treff.
  - Attverande treff er framleis venta i vedlikehaldsfiler og plan-/statusdokument; dette punktet kan først lukkast når vi anten snevrar søket til normalflate eller fjernar siste vedlikehaldsreferansar heilt.
- [x] `bash tools/verify_selvstendighet.sh` passerer.
- [x] `./bin/nc check` passerer.
- [x] `./bin/nc test` passerer.
- [x] `./bin/nc verify-omgang6b` passerer.
  - På Darwin/arm64 passerer host/NCB/ELF-determinisme og 6b.3 Gen1-bygg; djup ELF-køyring er framleis Linux x86_64-spesifikk.
- [x] Release kan bygges fra stage-0 seed og `.no` uten C/Python.
  - Verifisert lokalt med `bash tools/verify_seed_only.sh`, `./bin/nc selfcheck` og `bash tools/build_stage0_release_assets.sh`; normal `publish.yml` bruker seed → `build_norscode_native.sh` utan C/Python i release-jobbane.

## Aapne risikoer

- [x] Native ELF runtime har fremdeles svakheter rundt stor fil-I/O, map/list/json og compiler-bundle.
  - Delvis lukka: fil-I/O, map/list/json og native dispatch er no sterke nok til at `./bin/nc test` og `bash tools/verify_selvstendighet.sh` passerer lokalt.
- [ ] Stage-0 seed ma kunne reproduseres uten C-kilde i repoet.
- [ ] Ekte Gen1 ELF bundle-køyring på Linux x86_64 er framleis ikkje stabil; preset-bundle via `dist/norscode_native` er no verifisert, og CI prøver denne først, men fell framleis tilbake til transitional chunked source-NCB til den djupe ELF→ELF-køyringa er stabil.
  - Siste konkrete Linux-hypotese som er retta lokalt: `_start` lagra `envp` til feil slot. Runtimeen les `envp` frå `HEAP_VA + 8` (`0x600008`), og generatoren brukar no same kontrollslot i staden for siste heap-slot.
  - Neste konkrete Linux-feil som er retta lokalt: `_start` sa at han brukte `%r12` til å lese `argc/argv/envp`, men maskinkoden brukte faktisk `%rsp` etter diagnosemarkørane. Gen1 ELF emitterer no rett `%r12`-basert lesing.
- [x] Dokumentasjonen har mange historiske referanser som maa ryddes varsomt.
  - Historiske referansar er no merka som arkiv/vedlikehald der dei står att.
- [x] CI maa skille historikk i `archive/` fra aktiv normal flyt.
  - `tools/no_c_python_active_surface.sh` skil tracked aktiv flate frå `archive/`, maintainer-workflowar og lokale genererte regen-artefaktar.

## Kort regel

Hvis en normal kommando trenger C eller Python, er planen ikke ferdig.

Hvis C eller Python bare finnes som tydelig arkivert historikk, og CI hindrer at det blir aktivt igjen, er vi ferdige.
