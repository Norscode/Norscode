# V8400 Bundle start list-mutation patch

## Formål
Dette steget fjernar den mest presise kjende blokkeringa frå `selfhost/bundler.no`:
- tom liste
- `legg_til(...)`
- sende lista vidare til `bygg_bundle(...)`

Dette mønsteret vart bevist raudt i `v8200`.

## Endring
`bundler.start()` brukar ikkje lenger:
- `la reine_modular: liste<tekst> = []`
- `legg_til(reine_modular, ...)`

I staden:
- byggjer han opp ei rein tekststreng med trimma argument
- splittar først til slutt tilbake til liste
- sender så lista vidare til `bygg_bundle(...)`

## Kvifor dette er rett no
Dette er den kortaste kjende source-fiksen mot den konkrete feilen som framleis blokkerer:
- direkte bundle-inngang
- bootstrap refresh via `nc bundle`

## Avgrensing
Dette dokumentet påstår ikkje at aktiv `dist/norscode_native` alt har plukka opp endringa. Det påstår berre at bundle-inngangen i kjelda no ikkje brukar den raude liste-mutasjonsbana lenger.
