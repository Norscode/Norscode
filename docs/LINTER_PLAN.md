# Norscode Linter Plan

Målet er en enkel linter som fanger vanlige feil tidlig uten å bli støyende.

## Prinsipper

- Få regler først, men de skal være nyttige.
- Meldinger skal være konkrete og handlingsbare.
- Linteren skal supplere semantic-fasen, ikke duplisere alt den allerede gjør.

## Første regler

- Ubrukte imports.
- Dupliserte funksjonsnavn.
- Enkle navneskygger i lokal scope.
- Uoppnåelig kode etter `returner`, `kast`, `bryt` og `fortsett`.
- Unødvendige tomme blokker eller uttrykk uten effekt.

## Praktisk omfang

- Kjør som `norcode lint fil.no`.
- Støtt `--json` for maskinlesbar output.
- Støtt `--check` for CI-bruk.

## Implementasjonsrekkefølge

1. Bruk eksisterende parser og semantic-analyse til å samle symboler.
2. Legg inn de billigste reglene først.
3. Gjør output stabil og maskinlesbar.
4. Utvid med flere regler bare når de gir klart signal.

## Akseptansekriterier

- Linteren finner ekte feil uten mange falske positiver.
- Den passer i CI og lokal utvikling.
- Den forblir rask nok til å kjøre ofte.
