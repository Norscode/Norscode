# Pattern Matching Design

Første versjon av pattern matching i Norscode bør være liten, forutsigbar og lett å senke til eksisterende `hvis`-flyt.

## Mål

- gi et kort og lesbart alternativ til lange `hvis`/`ellers hvis`-kjeder
- støtte vanlige literal-matcher først
- holde semantics enkel nok til å kunne parity-portes senere

## Foreslått modell

- `match`-uttrykk eller -statement på én verdi
- `når`-grener med literal-mønstre
- `ellers` som fallback
- `_` som wildcard

## Første versjon

Vi starter med:

- heltall
- tekst
- bool
- wildcard `_`

Valgfritt senere:

- liste- og map-patterns
- guards
- destrukturering
- nested patterns

## Semantics

- bare første match vinner
- literal-mønstre sammenlignes med eksisterende likhetsoperator
- `ellers` er påkrevd hvis ingen matcher dekker alle tilfeller
- ukjent eller duplisert mønster bør gi tidlig semantic-feil der det er mulig

## Senkning

Den første implementasjonen kan senkes til nestede `hvis`-grener i parser-/IR-laget, slik at vi får:

- liten runtime-overflate
- lett debugbar kode
- samme oppførsel i interpreter, bytecode og C-bane

## Ut av scope for v1

- avansert mønstergjennomgang
- komplette algebraiske datatyper
- exhaustive checking på tvers av hele type-systemet

