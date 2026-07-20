# V8800 Active bundle refresh gap patch

## Formål
Dette steget patchar den mest sannsynlege aktive gapen mellom:
- grøn emulert bundle-bane
- raud direkte `run selfhost/bundler.no`-bane

## Endring
I `selfhost/nc_main.no`, inne i `nc_bundle_ncb(...)`, vart importkøa endra frå tom liste + `legg_til(...)` til tekstakkumulering + `builtin.split(...)` til slutt.

Før:
- `la kø: liste<tekst> = []`
- `legg_til(kø, ...)`

No:
- `la kø_str = ""`
- bygg opp med newline-separerte element
- `la kø = builtin.split(kø_str, "\n")`

## Kvifor dette er rett no
Direkte `run selfhost/bundler.no` går gjennom:
- `nc_run(...)`
- `nc_bundle_ncb(...)`

Sidan den emulerte bundle-bana allereie er grøn, var denne kø-initialiseringa den mest konkrete attverande mistenkte i aktiv bane.

## Avgrensing
Dette steget påstår ikkje at refresh eller direkte bundle no er grøn. Det påstår berre at den raude initialiseringa i import-bundlevegen er fjerna frå kjelda.
