# V8600 Refresh retry after bundle-start patch

## Formål
Dette steget prøver same refresh- og bundle-recheck-løype etter `v8400`-patchen i `bundler.start()`.

## Kva som vart prøvd
1. full refresh via `NORSCODE_V6000_MODE=--apply ./bin/nc run tools/refresh_bootstrap_compiler_v6000.no`
2. direkte `run selfhost/bundler.no` med `NORSCODE_BUNDLE_ARGS`
3. emulert patched branch via `tests/fixtures/v7400/bundler_single_module_emulated_v7400.no`

## Korleis lese resultatet
- om full refresh framleis er raud, sit bootstrap-regenereringa framleis fast i aktiv bane
- om direkte bundle framleis er raud, har ikkje aktiv `dist` plukka opp nok av source-fiksane enno
- om emulert branch er grøn, står hjelpar- og skrivebana framleis støtt
