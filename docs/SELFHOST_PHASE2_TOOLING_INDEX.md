# Selfhost Fase 2 - Verktøy og feilsøkingsindeks

Dette dokumentet peikar til dei konkrete inngangane som gjer fase 2 lettare å bruke og feilsøkje.

## Kva du bruker når noko er uklart

### Status og gate

- [docs/SELFHOST_PHASE2_STATUS.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_STATUS.md)
- [docs/SELFHOST_PHASE2_CI_STATUS.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_CI_STATUS.md)
- [docs/SELFHOST_PHASE2_MILESTONE_TEST_MAP.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_MILESTONE_TEST_MAP.md)

### Runtime

- [docs/SELFHOST_PHASE2_RUNTIME_TARGETS.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_RUNTIME_TARGETS.md)
- [docs/SELFHOST_PHASE2_RUNTIME_OPERATING_PLAN.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_RUNTIME_OPERATING_PLAN.md)
- [selfhost/runtime/production_runtime.no](/Users/jansteinar/Projects/Norscode1/selfhost/runtime/production_runtime.no)

### Diagnostikk

- [docs/SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md)
- [selfhost/compiler/runtime_sync.no](/Users/jansteinar/Projects/Norscode1/selfhost/compiler/runtime_sync.no)
- [selfhost/compiler/compiler_pipeline.no](/Users/jansteinar/Projects/Norscode1/selfhost/compiler/compiler_pipeline.no)
- [selfhost/compiler/semantic_correctness.no](/Users/jansteinar/Projects/Norscode1/selfhost/compiler/semantic_correctness.no)

### Standardbibliotek

- [docs/SELFHOST_PHASE2_STDLIB_LØYPE_V1.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_STDLIB_LØYPE_V1.md)
- [docs/SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md)
- [docs/SELFHOST_PHASE2_STDLIB_USECASES.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_STDLIB_USECASES.md)

### Test og benchmark

- [tests/test_selfhost_phase2_smoke.no](/Users/jansteinar/Projects/Norscode1/tests/test_selfhost_phase2_smoke.no)
- [tests/test_selfhost_phase2_ffi_smoke.no](/Users/jansteinar/Projects/Norscode1/tests/test_selfhost_phase2_ffi_smoke.no)
- [tests/test_selfhost_phase2_regression.no](/Users/jansteinar/Projects/Norscode1/tests/test_selfhost_phase2_regression.no)
- [tests/test_selfhost_phase2_stdlib_usecases.no](/Users/jansteinar/Projects/Norscode1/tests/test_selfhost_phase2_stdlib_usecases.no)
- [selfhost/phase2_ci.no](/Users/jansteinar/Projects/Norscode1/selfhost/phase2_ci.no)
- [selfhost/phase2_benchmark.no](/Users/jansteinar/Projects/Norscode1/selfhost/phase2_benchmark.no)

## Praktisk bruk

- Køyr `./bin/nc ci` for gate, status og baseline
- Les `docs/SELFHOST_PHASE2_TOOLING_INDEX.md` når du treng ein inngang til fase 2
- Les `docs/SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md` når du treng å forstå feilmønster
- Les `docs/SELFHOST_PHASE2_CI_STATUS.md` når du treng ein maskinlesbar gate

## Kva dette skal hjelpe med

- redusere leiting mellom statusdokument
- peike til rett test eller gate ved feil
- gi ein enkel startstad for ny fase-2-arbeid
