# v3003 Native candidate ready

Status: klar kandidat, ikkje promotert.

Dato: 2026-06-25T14:51:51Z

## Kandidat

- Fil: `build/v3002/norscode_native_v3002`
- Storleik: `1615304` bytes
- SHA256: `242c26c3aa4e3caa1f76aa81113bf0dbc7fd7070b6d799bbcd0759380409fa36`
- Plattform: macOS arm64 Mach-O

## Runtime-gap gate

Gate: `NORSCODE_NATIVE_GAP_BIN=build/v3002/norscode_native_v3002 ./bin/nc run tools/native_runtime_gap_gate_v3001.no`

Resultat:

```text
random_hex OK len=8
tid_ms OK
tid_no OK
now OK
timestamp OK
exec_prosess OK
[OK] runtime-gap gate er gronn
```

## Viktige grenser

- `dist/norscode_native` er ikkje endra.
- `bootstrap/stage0` er ikkje endra.
- Kandidaten er ikkje produksjons-promotert.
- `exec_prosess` er DEV-gated og krev `NORSCODE_ENABLE_EXEC_PROSESS=1`.
- `random_hex` er berre bekrefta i denne kandidaten, ikkje i aktiv `dist`.

## Neste trygge steg

1. v3004: lag eksplisitt promotering-notat for `dist`, men utan automatisk promotering.
2. v3005: bygg separat Linux-kandidat med same runtime-gap gate.
3. v3006: først etter eksplisitt ja kan `tools/promote_native_stage0_v3001.no` brukast.
