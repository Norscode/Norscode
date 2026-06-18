# Selfhost Fase 2 - Regresjonstestar

Dette dokumentet samlar dei viktigaste regresjonstestane for fase 2.

## Dekka område

- ABI og integrasjon
- runtime-statusflater
- standardbibliotek-statusflater
- CI-status og benchmark

## Testar

- [x] [tests/test_selfhost_phase2_smoke.no](/Users/jansteinar/Projects/Norscode1/tests/test_selfhost_phase2_smoke.no)
- [x] [tests/test_selfhost_phase2_ffi_smoke.no](/Users/jansteinar/Projects/Norscode1/tests/test_selfhost_phase2_ffi_smoke.no)
- [x] [tests/test_selfhost_phase2_regression.no](/Users/jansteinar/Projects/Norscode1/tests/test_selfhost_phase2_regression.no)

## Minimumskrav for kvar regresjonstest

- bruker berre normal Norscode-løype
- sjekkar minst eitt konkret leveransepunkt
- feilar tidleg med tydeleg årsak
- kan køyrast utan Python/C i normal utviklingsløype
