# Norscode Formatter Plan

Målet er én enkel formatter som gjør Norscode-kode lett å lese og lett å sammenligne i diff.

## Prinsipper

- Behold semantikk; formatteren skal aldri endre programoppførsel.
- Vær deterministisk; samme input skal alltid gi samme output.
- Prioriter lesbarhet foran kreativ omstrukturering.
- Støtt hele kodebasen gradvis, men ikke med halvformatert output.

## Første versjon

- Normaliser linjeskift til `\n`.
- Stram inn mellomrom rundt operatorer.
- Behold innrykk basert på blokknivå.
- Fjern ekstra tomlinjer der det er trygt.
- Behold kommentarer og strengliteraler så nære originalen som mulig.

## Praktisk omfang

- Kjør som `norcode format fil.no`.
- Støtt `--check` for å feile hvis filen ikke er formatert.
- Støtt `--diff` for å vise foreslått endring uten å skrive filen.

## Implementasjonsrekkefølge

1. Bygg en stabil AST-til-tekst-rutine.
2. Legg på `format`-kommando i CLI.
3. Legg på `--check` og `--diff`.
4. Kjør formatteren på eksempelfiler og testfiler.

## Akseptansekriterier

- Formatteren er deterministisk.
- Output er lesbar på tvers av alle støttede språkformer.
- Vi kan kjøre formatteren på eksisterende `.no`-filer uten å miste informasjon.
