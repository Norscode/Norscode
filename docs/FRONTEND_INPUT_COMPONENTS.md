# Frontend Input Components

Input-komponenter er grunnlaget for skjemaer i frontend.

## Mål

- gjøre inputfelt konsistente
- holde labels, hjelpetekst og feil tett på feltet
- gjøre skjemaer enkle å lese og bruke

## Komponenter

- tekstfelt
- tallfelt
- passordfelt
- checkbox
- radio-grupper
- select-lister
- tekstområde

## Regler

- hvert felt bør ha tydelig label
- hjelp og feilmelding skal være nær feltet
- bruk samme stil og oppførsel på tvers av appen
- felt bør kunne deaktiveres og markeres som loading
- bygg felt med `std.html.form(...)`, `std.html.label(...)`, `std.html.input(...)`, `std.html.textarea(...)` og `std.html.select(...)` der det passer

## Hva et felt bør støtte

- navn
- label
- placeholder
- verdi
- disabled
- readonly
- feiltekst
- hjelpetekst

## Når dette er ferdig

- skjemaer kan bygges av standardfelt
- brukeropplevelsen blir konsistent

Se også [docs/FRONTEND_COMPONENT_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_COMPONENT_MODEL.md).
