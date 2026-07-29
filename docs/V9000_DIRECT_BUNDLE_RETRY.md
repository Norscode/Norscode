# V9000 Direct bundle retry

## Formål
Dette steget re-testar den direkte bundle-bana etter patchen i `nc_bundle_ncb(...)`.

## Kva som vart prøvd
1. direkte `run selfhost/bundler.no` med éin modul i `NORSCODE_BUNDLE_ARGS`
2. full refresh via `NORSCODE_V6000_MODE=--apply ./bin/nc run tools/refresh_bootstrap_compiler_v6000.no`

## Kva vi les ut
- om direkte bundle framleis er raud, ligg det framleis minst eitt aktivt stack-underflow-punkt att under dagens kjeldepatchar
- om refresh framleis er raud, er bootstrap-regenerering framleis blokkert i aktiv bane
