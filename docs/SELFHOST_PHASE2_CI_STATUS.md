# Selfhost Fase 2 - CI-status

Dette dokumentet er ei kort, maskinlesbar vennleg statusflate for fase 2.

## Kort status

- [x] ABI-minimum v1 er definert
- [x] Runtime-API v1 er kartlagt
- [x] Kall-kontraktar for builtin og extern-modular er dokumentert
- [x] FFI-smoketest er på plass
- [x] Standardbibliotek-løypa har prioriterte modular
- [x] Fase-2-smoketest for std-modular er lagt inn
- [x] Full CI-gate for fase 2 har ein fast rapportflate

## Gate-nivå

### Klar

- `docs/SELFHOST_PHASE2_ABI_MINIMUM_V1.md`
- `docs/SELFHOST_PHASE2_RUNTIME_API_V1.md`
- `docs/SELFHOST_PHASE2_CALL_CONTRACTS_V1.md`
- `docs/SELFHOST_PHASE2_FFI_SMOKETEST_V1.md`
- `docs/SELFHOST_PHASE2_STDLIB_LØYPE_V1.md`
- `docs/SELFHOST_PHASE2_DOD.md`
- `docs/SELFHOST_PHASE2_REGRESSION_TESTS.md`
- `tests/test_selfhost_phase2_smoke.no`
- `tests/test_selfhost_phase2_regression.no`

### Under arbeid

- `docs/SELFHOST_PHASE2_STATUS.md`
- `docs/SELFHOST_PHASE2_BACKLOG.md`
- `docs/SELFHOST_HANDLINGSPLAN.md`

### Manglar

- Ingen fase-2 manglar i CI-statusen; `./bin/nc ci` viser no baseline benchmark før testløypa.

## Foreslått minimumsrapport

Når fase 2 er køyrt i CI, bør rapporten minst vise:

```text
phase2.abi=ok
phase2.runtime_api=ok
phase2.call_contracts=ok
phase2.ffi_smoke=ok
phase2.stdlib_smoke=ok
phase2.ci_gate=warn|ok|fail
```

## Akseptkriterium

- [x] Statusen er enkel å lese for menneske
- [x] Statusen kan oversetjast til ei maskinlesbar gate
- [x] Statusen er kopla direkte til `./bin/nc ci`
- [x] Statusen gir eitt eintydig svar om fase 2 kan sleppast vidare
