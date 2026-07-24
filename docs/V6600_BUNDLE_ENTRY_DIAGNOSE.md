# V6600 Bundle entry diagnose

## Resultat
Tre skilje vart testa:

1. Vanleg compile av `selfhost/lexer/lexer_m1.no`
   - grøn
2. `run selfhost/bundler.no` utan bundle-args
   - gir normal funksjonsfeil: `Feil: NORSCODE_BUNDLE_ARGS ikkje sett`
3. `run selfhost/bundler.no` med éin modul i `NORSCODE_BUNDLE_ARGS`
   - feilar med `norscode: Stack underflow`

## Tolkning
Dette snevrar inn feilen meir enn `v6400` gjorde:
- `dist/norscode_native` compile-løype er ikkje generelt død
- `bundler.start` klarer å starte og lese miljøvariablar
- stack-underflow skjer først når `bygg_bundle(...)` faktisk blir kalla med ekte modul-liste

## Ny minste reproduksjon
`NORSCODE_CMD=run NORSCODE_FILE=selfhost/bundler.no NORSCODE_BUNDLE_ARGS='selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no'`

## Neste rette steg
Vidare diagnose må no skilje mellom:
- `bygg_bundle` sin single-module snarveg
- `les_ncb_eller_kompiler`
- `kompiler_fil(kjelde, "__main__")` slik ho blir kalla frå bundler-sporet
