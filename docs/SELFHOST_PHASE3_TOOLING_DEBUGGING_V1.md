# Selfhost Fase 3 - Verktøy og feilsøkingsstøtte v1

Dette dokumentet samlar den første fase-3-flata for verktøy og feilsøkingsstøtte.

## Formål

- Gjere feilsøking raskare og meir forutsigbar
- Skilje parser-, semantic- og IR-feil tydeleg
- Ha ei kort og lesbar statusflate for utviklarar

## Hovudområde

### Diagnostikk

- skilje mellom parser-feil, semantic-feil og IR-feil
- bruke posisjonsinformasjon der det finst
- halde feilmeldingar deterministiske

### Status

- vise kva som er ferdig, pågår og manglar
- ha ei kort maskinlesbar linje for fase 3
- vere lett å skumlese i repoet

### Feilsøking

- peike til den rette dokumentasjonen først
- samle referansar for smoke og regresjon
- gjere det klart kvar ein skal starte når noko feilar

## Byggjer på

- [docs/SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md)
- [docs/SELFHOST_PHASE2_TOOLING_INDEX.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_TOOLING_INDEX.md)
- [docs/SELFHOST_PHASE3_STATUS.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE3_STATUS.md)

## Kva som er klart

- Diagnosevegen for fase 3 er no definert som eigen arbeidsflate
- Status- og referansedokument er alt på plass

## Vidare arbeid

- eventuelle meir konkrete feilmeldingsformat i nye pass
- oppdatering av indeksar når nye verktøy kjem
