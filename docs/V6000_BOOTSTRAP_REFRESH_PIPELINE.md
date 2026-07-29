# V6000 Bootstrap refresh pipeline

## Formål
Dette steget samlar den faktiske regenereringsløypa for den gamle bundlede/bootstrap-compileren i eitt trygt skript.

Bakgrunn:
- `selfhost/kompiler.no` peikar allereie på oppdatert `selfhost/compiler/ir_to_bytecode.no`
- problemet sit derfor ikkje primært i kjeldekoden lenger
- problemet sit i at den aktive bundlede/bootstrap-compileren ikkje nødvendigvis er regenerert frå desse kjeldene

## Verktøy
- `tools/refresh_bootstrap_compiler_v6000.no`

### Modusar
- `--dry-run`
  - viser nøyaktig kva som skal skje
  - endrar ingen filer
- `--apply`
  - køyrer regenereringa på ordentleg vis

## Pipeline
1. `NORSCODE_REGEN_BOOTSTRAP_FULL=1 ./bin/nc run tools/nc_regen_bootstrap.no`
2. `selfhost/tooling/regenerate_omgang6b_fragments.no`
3. `tools/build_omgang6b_compiler_ncb.no`

## Etterpå
Når refresh er køyrd, må ein bruke:
- `tools/run_bootstrap_literal_probes_v5600.no`
- `tools/bootstrap_codegen_diff_v5800.no`

for å sjå om aktiv bootstrap/codegen no faktisk emitterer:
- `BUILD_LIST`
- `BUILD_MAP`
- `INDEX_GET`

## Viktig
Dette steget påstår ikkje at bootstrap-compileren er fiksa. Det gjer berre refresh-løypa repeterbar og tryggare å bruke i den neste patch-runden.
