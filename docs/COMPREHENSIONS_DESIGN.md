# Comprehensions

Målet er å få en kompakt syntaks for å bygge lister og ordbøker uten å miste lesbarhet.

## Valgt retning

- Start med liste-comprehensions.
- Bruk eksplisitt, norsk venstredel og et enkelt filter.
- Hold første versjon fri for sideeffekter.

## Foreslått syntaks

- ` [uttrykk for navn i samling hvis betingelse] `
- ` [uttrykk for navn i samling] `

## Prinsipper

- Syntaksen skal være lett å lese i kodegjennomgang.
- Den skal kunne oversettes til eksisterende `for`- og `hvis`-primitiver.
- Første versjon skal være enkel nok til å bli støttet likt i Python-, bytecode- og C-banen.

## Første implementasjonssteg

- Parser for liste-comprehensions.
- AST-node for kompakt samlingsuttrykk.
- Semantic-sjekk for uttrykk, iterator og filter.
- Runtime og codegen som ekspanderer til vanlig løkke.
