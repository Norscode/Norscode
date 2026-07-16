# v3002 native candidate build status

Dette dokumenterer neste steg etter v3001 runtime-gap-gate.

## Status

- `tools/build_native_candidate_v3002.no` byggjer ein isolert kandidat under `build/v3002/`.
- Kandidaten promoterer ikkje til `dist/` eller `bootstrap/stage0/`.
- Kandidatsporet brukar legacy maintainer-generatoren `archive/legacy_c_backend/ncb_to_c.no`.
- `archive/legacy_c_backend/nc_runtime_mini.c` har no runtime-implementasjonar for:
  - `builtin.random_hex`
  - `builtin.exec_prosess`
  - `builtin.tid_ms`
  - `builtin.tid_no`
  - `builtin.now`
  - `builtin.timestamp`

## Blokkering

Full kandidatbygg er framleis blokkert fordi aktiv kompilator ikkje tek med sluttseksjonen av `selfhost/compiler/ir_to_bytecode.no` i NCB-output.

Manglande funksjonar i produsert NCB:

- `kompiler_funksjon`
- `kompiler_test`
- `kompiler_program`
- `kompiler_til_ncb_json`
- `start`

Dette gjer at full native-host ikkje kan byggjast grønt enno.

## Trygg regel

Ingen runtime eller stage0 skal promoterast før:

```sh
./bin/nc run tools/build_native_candidate_v3002.no
NORSCODE_NATIVE_GAP_BIN=build/v3002/norscode_native_v3002 ./bin/nc run tools/native_runtime_gap_gate_v3001.no
```

begge går grønt.
