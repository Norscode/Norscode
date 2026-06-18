# Selfhost Fase 2 - Standardbibliotek-brukscase

Dette dokumentet viser dei praktiske fase-2-brukstilfella for dei prioriterte modulane.

## `std/log.no`

- lag strukturert logg med nivå og melding
- filtrer loggar på nivå eller felt
- tel loggar for enkel statusrapport

## `std/fil.no`

- sjekk om ei fil finst
- les fil inn på trygg måte
- få ei rask statusoversikt for fil og linjetal

## `std/cache.no`

- opprett ein minnecache
- set og hent verdiar med TTL
- rydd utløpte nøklar og vis status

## `std/lagring.no`

- lagre JSON til fil
- last data frå JSON-fil
- hent tekst, tal eller status frå lagra data

## `std/innstillingar.no`

- opprett ei tom konfigurasjonskjelde
- les miljønøkkeloversikt
- sjå kva som er tilgjengeleg i status

## `std/sched.no`

- legg inn planlagde hendingar
- køyr ein enkel scheduler-syklus
- sjå neste hending og ventetid

## `std/tråd.no`

- opprett ein trådmanager
- sjå status for aktive og blokkerte trådar
- les oversikt utan å gå inn i intern tilstand

## Praktisk leseregel

- Bruk dette dokumentet som "kva kan eg gjere med modulen i dag?"
- Bruk statusmatrisa som "kva er moden nok til fase 2?"
