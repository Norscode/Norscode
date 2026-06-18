# Selfhost Fase 2 - Milepæl til test-kart

Dette dokumentet bind fase-2-milepælane til konkrete regresjonar og statusflater.

## ABI og integrasjon

- S2-01 ABI-minimum v1
  - verna av `docs/SELFHOST_PHASE2_ABI_MINIMUM_V1.md`
  - verna av `docs/SELFHOST_PHASE2_RUNTIME_API_V1.md`
  - verna av `docs/SELFHOST_PHASE2_CALL_CONTRACTS_V1.md`
  - verna av `docs/SELFHOST_PHASE2_MIGRATION_NOTE.md`
  - røykdekt av `tests/test_selfhost_phase2_ffi_smoke.no`

## Runtime og verktøy

- S2-03 Runtime-stabiliseringsmål
  - verna av `docs/SELFHOST_PHASE2_RUNTIME_TARGETS.md`
  - verna av `docs/SELFHOST_PHASE2_RUNTIME_OPERATING_PLAN.md`
  - statusdekka av `docs/SELFHOST_PHASE2_CI_STATUS.md`
  - røykdekt av `tests/test_selfhost_phase2_smoke.no`
  - rapportert i `selfhost/phase2_ci.no` og `selfhost/phase2_benchmark.no`

## Standardbibliotek

- S2-02 Standardbibliotek-løype v1
  - verna av `docs/SELFHOST_PHASE2_STDLIB_LØYPE_V1.md`
  - verna av `docs/SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md`
  - røykdekt av `tests/test_selfhost_phase2_smoke.no`
  - regresjonsdekt av `tests/test_selfhost_phase2_regression.no`

## Kvalitet og CI

- S2-04 CI-status v1
  - verna av `docs/SELFHOST_PHASE2_CI_STATUS.md`
  - brukt av `./bin/nc ci`
  - baseline-målt av `selfhost/phase2_benchmark.no`

- S2-05 DoD og regresjonar
  - verna av `docs/SELFHOST_PHASE2_DOD.md`
  - verna av `docs/SELFHOST_PHASE2_REGRESSION_TESTS.md`
  - verna av dette dokumentet
  - røykdekt av `tests/test_selfhost_phase2_smoke.no`
  - røykdekt av `tests/test_selfhost_phase2_ffi_smoke.no`
  - regresjonsdekt av `tests/test_selfhost_phase2_regression.no`

## Praktisk lesing

- Om ein milepæl står her med testreferanse, skal vi ha minst éin automatisk kontroll på den
- Om ein milepæl berre står med dokumentreferanse, er han framleis i definert arbeid, men ikkje utan verneflate
- Dette dokumentet skal vere den første staden ein ser etter når ein spør: “Kva testar vernar denne milepælen?”
