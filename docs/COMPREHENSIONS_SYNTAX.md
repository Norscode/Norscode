# Comprehensions: Valgt Syntaks

Første versjon bruker liste-comprehensions med norsk nøkkelordflyt.

## Syntaks

- `[` `uttrykk` `for` `navn` `i` `samling` `hvis` `betingelse` `]`
- `[` `uttrykk` `for` `navn` `i` `samling` `]`

## Eksempler

- `[x * 2 for x i tall]`
- `[navn for navn i navn hvis navn != ""]`
- `[tekst_fra_heltall(x) for x i tall hvis x > 0]`

## Semantikk

- `navn` er en lokal iterasjonsvariabel i uttrykket.
- `samling` må være en liste i første versjon.
- `hvis`-delen er valgfri og filtrerer elementer.
- Resultatet er en ny liste og har ingen sideeffekter.

## Hvorfor denne formen

- Den er lesbar for eksisterende Norscode-brukere.
- Den kan senere ekspanderes direkte til en vanlig løkke i compiler-banen.
- Den holder første versjon enkel nok til å porteres mellom Python-, bytecode- og C-banen.
