# Full Independence Inventory

Dette dokumentet listar dei gjenværande referansane som må vurderast før Norscode kan kallast fullt uavhengig utan nokon legacy-/maintainer-beslag i det heile.

Målet er ikkje å slette historikk, men å skilje heilt tydeleg mellom:

- normal drift
- eksplisitt vedlikehald
- arkivert legacy

## 1. Aktiv normaldokumentasjon og statusdocs som framleis omtalar legacy-/maintainer-spor

### README

- `/Users/jansteinar/Projects/Norscode1/README.md`
  - omtalar framleis vedlikehaldsbru for seed-fornying
  - omtalar at C-bruk er avgrensa til vedlikehald

### ROADMAP

- `/Users/jansteinar/Projects/Norscode1/ROADMAP.md`
  - peikar til fase 0 som open ryddings-/CI-milepæl
  - lenkar til den nye full-independence-sjekklista

### Migrasjon/deprecations

- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_MIGRATION_AND_DEPRECATIONS.md`
  - omtalar vedlikehalds-C som historisk/isolert
  - held fram med å beskrive bootstrap-/maintainer-lane som historisk referanse

### Andre aktive docs som framleis nemner legacy

- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_CI_GATES.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_RELEASE_CHECKLIST.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_FALLBACK_CONTRACT.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_DEPENDENCY_MAP.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/NATIVE_CODEGEN_V2_ABI.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_MIGRATION_AND_DEPRECATIONS.md`

### Status: allereie rydda til historisk/isolert språk

- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_CI_GATES.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_RELEASE_CHECKLIST.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_FALLBACK_CONTRACT.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_DEPENDENCY_MAP.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/NATIVE_CODEGEN_V2_ABI.md`
- `/Users/jansteinar/Projects/Norscode1/docs/05-development/SELFHOST_MIGRATION_AND_DEPRECATIONS.md`

## 2. Workflow-/tool-flate som framleis inneheld maintainer-referansar

### CI / workflow

- `/Users/jansteinar/Projects/Norscode1/.github/workflows/regen_bootstrap.yml`
  - explicit `Regen-verify (Linux maintainer-lane)`
  - bruker `NORSCODE_BOOTSTRAP_C=1 REGEN=1`

- `/Users/jansteinar/Projects/Norscode1/.github/workflows/ci.yml`
  - har framleis maintainerspor for REGEN-bygg
  - refererer til bootstrap-/maintainer-kommandoar

- `/Users/jansteinar/Projects/Norscode1/.github/workflows/export-stage0-linux.yml`
  - eksplisitt migrerings-/stage0-arbeidsflyt

### Status: workflow-filene er historisk merkte, men framleis operative vedlikehaldsvegar

- `/Users/jansteinar/Projects/Norscode1/.github/workflows/regen_bootstrap.yml`
- `/Users/jansteinar/Projects/Norscode1/.github/workflows/ci.yml`
- `/Users/jansteinar/Projects/Norscode1/.github/workflows/export-stage0-linux.yml`

### Tools

- `/Users/jansteinar/Projects/Norscode1/tools/build_norscode_native.sh`
  - inneheld maintainer-modus med `REGEN=1`
  - inneheld bootstrap/stage0-seed-løypa

- `/Users/jansteinar/Projects/Norscode1/tools/maint/bootstrap.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/regen_native.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/regen_verify.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/verify_l6.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/ensure_stage0_seed.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/migrate_bootstrap_c_to_stage0.sh`

### Status: maintainer-/bootstrap-skript er historisk merkte, men framleis tilgjengelege

- `/Users/jansteinar/Projects/Norscode1/tools/build_norscode_native.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/bootstrap.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/regen_native.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/regen_verify.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/verify_l6.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/ensure_stage0_seed.sh`
- `/Users/jansteinar/Projects/Norscode1/tools/maint/migrate_bootstrap_c_to_stage0.sh`

### Stage-0 / bootstrap

- `/Users/jansteinar/Projects/Norscode1/bootstrap/stage0/README.md`
- `/Users/jansteinar/Projects/Norscode1/bootstrap/maint/c/README.md`
- `/Users/jansteinar/Projects/Norscode1/bootstrap/maint/c/norscode_generated.c`
- `/Users/jansteinar/Projects/Norscode1/bootstrap/stage0/norscode-linux-x86_64`
- `/Users/jansteinar/Projects/Norscode1/bootstrap/stage0/norscode-macos-arm64`
- `/Users/jansteinar/Projects/Norscode1/bootstrap/stage0/norscode-*.sha256`

## 3. Arkiv som framleis må vere tydeleg merkte som historikk

### Legacy C backend / gamle bootstrap-artefakt

- `/Users/jansteinar/Projects/Norscode1/archive/legacy_c_backend/*`
- `/Users/jansteinar/Projects/Norscode1/archive/c_minimal_vm/*`

### Historiske dokument

- `/Users/jansteinar/Projects/Norscode1/docs/_archive/*`
- `/Users/jansteinar/Projects/Norscode1/.github/releases/v1.0-selfhost.md`
- `/Users/jansteinar/Projects/Norscode1/docs/INDEX.md`

## 4. Konkret rydde-rekkefølgje

1. Fjern eller omformulér alle aktive docs som framstiller maintainer-/bootstrap-bru som ein del av normal bruk.
2. Tydleggjer workflow-filene slik at maintainer-lane berre er historikk eller eksplisitt vedlikehald.
3. Rydd `tools/maint/*` ut av normal orientering og inn i vedlikehalds-/historikkspor.
4. Flytt eller merk bootstrap/stage0 og bootstrap/maint/c som historiske vedlikehaldsartefakt.
5. Verifiser med søk at normaldocs ikkje lenger inneheld:
   - `maintainer`
   - `seed-fornying`
   - `NORSCODE_BOOTSTRAP_C`
   - `REGEN=1`
   - `bootstrap/maint/c`

## 5. Akseptkriterium

Norscode kan kallast fullt uavhengig når:

- normal README/ROADMAP/docs ikkje peikar på legacy-/maintainer-flate som nødvendig drift
- CI ikkje krev maintainerspor for å vere grønn
- bootstrap/seed-vedlikehald berre finst som eksplisitt historikk/vedlikehald
- normal brukarflate er berre `./bin/nc` og `dist/norscode_native`
