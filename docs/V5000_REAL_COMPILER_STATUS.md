# V5000 real compiler status

## Samandrag

Dette steget samlar den nye `*_real`-kjeda til eit meir komplett compiler-spor.

Kjeda har no støtte i kjelde for:

- liste-literal
- map-literal
- indeks-uttrykk
- assignment / `STORE_VAR`

## Kva som no finst i `*_real`-sporet

Parser:
- `[]`
- `[1,2]`
- `{}`
- `{"x": 1}`
- `a[1]`
- `a = ...`

Typed IR:
- `BUILD_LIST`
- `BUILD_MAP`
- `INDEX_GET`
- `STORE_VAR`

Bytecode emitter:
- `BUILD_LIST`
- `BUILD_MAP`
- `INDEX_GET`
- `STORE_VAR`

VM runtime:
- `BUILD_LIST`
- `BUILD_MAP`
- `INDEX_GET`
- `STORE_VAR`

## Probe-sett

Grunnprober:
- `selfhost/probes_real/list_probe_real.no`
- `selfhost/probes_real/empty_list_probe_real.no`
- `selfhost/probes_real/map_probe_real.no`
- `selfhost/probes_real/index_probe_real.no`

Assignment-prober:
- `selfhost/probes_real/assignment_list_probe_real.no`
- `selfhost/probes_real/assignment_map_probe_real.no`
- `selfhost/probes_real/assignment_index_probe_real.no`

Køyrescript:
- `tools/run_real_pipeline_probes_v5000.no`

## Viktig presisering

Dette betyr ikkje at aktiv `dist/norscode_native` er fiksa.

Det betyr at den nye `*_real`-kjeda no er mykje nærmare ein komplett referanse for
korleis compile-løypa faktisk bør oppføre seg.

## Det som står att etter v5000

1. Køyre og verifisere probe-settet.
2. Lage ein liten fasit for venta bytecode:
   - `BUILD_LIST`
   - `BUILD_MAP`
   - `INDEX_GET`
   - `STORE_VAR`
3. Overføre same logikk til den aktive bootstrap/generated-C compile-løypa.
