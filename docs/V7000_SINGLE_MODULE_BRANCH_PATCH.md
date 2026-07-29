# V7000 Single-module branch patch

## Endring
Single-module-grena i `selfhost/bundler.no` brukar no `les_ncb_eller_kompiler(fil_sti_0)` i staden for å ha ei eiga direkte løype med:
- `fil_les(...)`
- `kompiler_fil(...)`

## Kvifor
I `v6800` vart dette slått fast:
- `les_precompiled_ncb(...)` er grøn
- `les_ncb_eller_kompiler(...)` er grøn
- single-module-grena i `bygg_bundle(...)` er raud

Den tryggaste første fiksen er derfor å fjerne spesialløypa og bruke den allereie beviste hjelpefunksjonen.

## Forventa effekt
Dette bør fjerne skiljet mellom:
- "compile fungerer via bundler-hjelparen"
- "single-module snarveg krasjar"

## Avgrensing
Dette dokumentet påstår ikkje at feilen er verifisert borte. Det påstår berre at den raude grena no er kopla over på den grøne hjelpebana.
