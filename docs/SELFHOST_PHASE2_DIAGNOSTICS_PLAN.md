# Selfhost Fase 2 - Diagnostikkplan for parser, semantic og IR

Dette dokumentet samlar den første konkrete planen for feilsøkingsstøtte i fase 2.

## Mål

- gjere parser-, semantic- og IR-feil lettare å skilje frå kvarandre
- få meir stabile og lesbare feilmeldingar
- vise i CI kvar i løypa ein feil oppstår

## Diagnostikknivå

### Parser

Målet er å gjere syntaksfeil konkrete.

- meldinga skal seie kva som manglar eller er ugyldig
- feilen skal kunne knytast til ein bestemt del av input
- parseren skal feile deterministisk

Minimum:

- `parse`-feil skal ikkje bli rapportert som generelle runtime-feil
- kvar parserfeil skal ha eit stabilt kategoriord

### Semantic

Målet er å gjere type- og regelfeil lett å lese.

- feilen skal vise kva regel som vart broten
- det skal vere klart om feilen er mjuk eller hard
- semantic-laget skal kunne gi fleire feil utan å miste hovudårsaka

Minimum:

- semantic-feil skal ha kategori og kort forklaring
- deterministisk rekkjefølgje på rapporterte feil

### IR

Målet er å gjere mellomrepresentasjon-feil sporbart.

- feilen skal vise om problemet ligg i generering, loweringa eller valideringa
- IR-feil skal vere enkle å samanlikne i CI
- manglande IR skal rapporterast som eigen tilstand

Minimum:

- `IR mangler` skal vere eigen og tydeleg feilkategori
- IR-feil skal kunne koplast til fase-2-regresjonar

## CI-visning

I CI bør diagnostikken minst vise:

- fase
- kategori
- kort feilmelding
- om feilen kjem frå parser, semantic eller IR

## Prioriteringsrekkefølgje

1. Parserfeil med kategori og kort tekst
2. Semantic-feil med deterministisk ordning
3. IR-feil med eigen feilkategori

## Akseptkriterium

- ein lesar skal kunne sjå kva for lag feilen kjem frå
- feilmeldingane skal vere stabile nok til å brukast i CI
- planen skal vere kort nok til å fungere som arbeidsseddel
