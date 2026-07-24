# v3001 native runtime fix status

Dette er status etter første trygge forsøk på å lukke native/stage0 runtime-gap.

## Kva er fiksa i kjelde

Legacy maintainer-kjelda under `archive/legacy_c_backend/` har fått C-implementasjonar for:

- `builtin.random_hex(n)`
- `builtin.tid_ms()`
- `builtin.tid_no()`
- `builtin.now()` / `builtin.now_iso()`
- `builtin.timestamp()` / `builtin.unix_timestamp()`
- `builtin.exec_prosess(cmd)` med DEV-gate via `NORSCODE_ENABLE_EXEC_PROSESS=1`

Dette er ikkje lagt inn i aktiv produksjonsflyt. Det er med vilje, fordi prosjektregelen seier at C-sporet er legacy/maintainer-lane.

## Kva er ikkje promotert

- `dist/norscode_native` er ikkje overskrive.
- `bootstrap/stage0` er ikkje overskrive.
- `random_hex` skal framleis ikkje kallast produksjonsklar i aktiv CLI før gate-testen går grønt mot aktiv binary.

## Gate

Bruk:

```sh
cd "/Users/jansteinar/Norscode AI/prosjekter/Norscode"
./bin/nc run tools/native_runtime_gap_gate_v3001.no
```

Grønn gate krev at aktiv binary faktisk køyrer probe-fila utan `FEIL`:

```text
tests/native_runtime_gap_probe_v3001.no
```

## Sperre lagt inn

- `tools/native_runtime_gap_gate_v3001.no` er autoritativ runtime-gap-gate.
- `tools/build_norscode_native.no` kan køyrast med `NORSCODE_REQUIRE_RUNTIME_GAP=1` for å nekte binærar som berre passerer enkel smoke.
- `tools/promote_native_stage0_v3001.no` nektar promotering til `dist/` eller `bootstrap/stage0/` før kandidaten passerer runtime-gap-gate.

## Neste trygge steg

1. Regenerer ein maintainer-kandidat i `build/`.
2. Køyr `NORSCODE_NATIVE_GAP_BIN=<kandidat> ./bin/nc run tools/native_runtime_gap_gate_v3001.no`.
3. Først når kandidaten er grønn: be eksplisitt om promotering til `dist/norscode_native`.
4. Stage0-seed skal berre oppdaterast som separat release-steg med backup og checksum.
