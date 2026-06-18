# Selfhost Fase 2 - Standardbibliotek-løype v1

Dette dokumentet peikar no til den samla statusmatrisa for standardbiblioteket.

## Priortert løype

- [x] `std/log.no`
- [x] `std/fil.no`
- [x] `std/cache.no`
- [x] `std/lagring.no`
- [x] `std/innstillingar.no`
- [x] `std/sched.no`
- [x] `std/tråd.no`

## Kva vi ser etter

- [x] Klar API-flate
- [x] Tydeleg null-/feilkontrakt
- [x] Minst éin praktisk brukscase per modul
- [x] Låg friksjon for normal utvikling

## Første status

- Statusmatrisa ligg i [docs/SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md)
- `std/log.no` er styrkt med betre validitet, fleire felt, tidsstempel, filtrering og statusflate
- `std/fil.no` er styrkt med tryggare inputkontroll og statusflate
- `std/cache.no` er styrkt med betre gyldigheitskontroll, opprydding av utløpte nøklar og enkel statusflate
- `std/lagring.no` er styrkt med reell JSON-last og tryggare skriv/les-kontrakt
- `std/innstillingar.no` er styrkt med statusflate og miljønøkkeloversikt
- `std/sched.no` er styrkt med statusflate for kø og neste hending
- `std/tråd.no` er styrkt med statusoversikt for manager og trådstatus
- Minimumstest for fase 2 er lagt inn i [tests/test_selfhost_phase2_smoke.no](/Users/jansteinar/Projects/Norscode1/tests/test_selfhost_phase2_smoke.no)

## Ferdig når

- [x] Dei prioriterte modulane har stabile hjelparar og enkel status
- [x] Fase 2-planen kan peike på modulane utan ekstra forklaring
- [x] Teamet kan sjå kva som er på plass med eitt blikk
