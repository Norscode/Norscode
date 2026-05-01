# Generiske containere

Målet er å få en enkel og forutsigbar generics-modell som passer Norscode sin nåværende arkitektur.

## Valgt modell

- Generics monomorfiseres ved kompilering.
- Typeargumenter er eksplisitte i syntaksen.
- Det finnes ingen runtime-refleksjon over typeparametere i første versjon.
- Feil skal meldes tidlig i semantic-fasen, før kodegenerering.

## Foreslått syntaks

- `funksjon ident<T>(verdi: T) -> T { ... }`
- `liste<T>`
- `ordbok<K, V>`

## Prinsipper

- Hold modellen enkel nok til å fungere i både Python-, bytecode- og C-banen.
- Unngå type-erasure der det skaper uklare feil eller treg runtime.
- Gjør spesialisering deterministisk og lett å debugge.

## Første implementasjonssteg

- Parser for typeparametere i deklarasjoner og typeannotasjoner.
- Semantic-sjekk for at typeargumenter er gyldige og konsistente.
- Monomorfisering av generiske funksjoner og containere i codegen.
- Parity-tester i selfhost når første versjon er stabil.

## Levert først

- Syntaks for `liste<heltall>`, `liste<tekst>` og `ordbok<tekst, T>` er nå parse- og semantikkstøttet.
- De nye formene mapper til de eksisterende container-typene i dagens runtime og codegen.
- Dette er første, smale generics-trinn. Full monomorfisering for vilkårlige typeparametere kommer senere.
