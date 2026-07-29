# Lambdaer og closures

Målet er å få korte anonyme funksjoner som kan lukke over ytre variabler uten at språket blir uklart.

## Valgt retning

- Lambdaer skal være uttrykk, ikke statements.
- De skal kunne referere til variabler fra nærmeste ytre scope.
- Første versjon skal være ren og forutsigbar, uten avansert muterbar closure-state.

## Foreslått syntaks

- `fun(x) -> x + 1`
- `fun(x, y) -> { returner x + y }`
- `fun() -> verdi`

## Prinsipper

- Lambdaer skal være en lettvektsvariant av vanlige funksjoner.
- Closure-oppsett skal være tydelig nok til å fungere i både tolk, bytecode og C-bane.
- Første versjon skal kunne oversettes til en intern funksjonsreferanse med fange-scope.

## Første implementasjonssteg

- Parser for lambda-uttrykk.
- AST-node for anonym funksjon.
- Semantic-sjekk for parametere, returtype og fri variabel-bruk.
- Runtime/codegen for closure-capture.
