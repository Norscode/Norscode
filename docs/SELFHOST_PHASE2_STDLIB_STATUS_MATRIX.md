# Selfhost Fase 2 - Standardbibliotek-statusmatrise

Dette er ei rask oversikt over kva som er klart for bruk i fase 2, kva som er på veg, og kva som ligg utanfor prioritet akkurat no.

## Kan brukast i prosjekt no

- `std/log.no` - statusflate, filtrering og feltstyring
- `std/fil.no` - tryggare filhandtering og status
- `std/cache.no` - status, opprydding og nøkkeloversikt
- `std/lagring.no` - JSON-last og tryggare skriv/les
- `std/innstillingar.no` - miljønøkkeloversikt og status
- `std/sched.no` - køstatus og neste hending
- `std/tråd.no` - manager- og trådstatus
- praktisk brukscase-dekning ligg i [docs/SELFHOST_PHASE2_STDLIB_USECASES.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_STDLIB_USECASES.md)

## Under arbeid

- breiare standardbibliotek utover fase-2-løypa
- lengre regresjonssett per modulområde

## Manglar

- ei full standardbibliotek-paritet mot Python
- samla dokumentasjon for alle ikkje-prioriterte modular
- eit endå breiare testnett for moduler som ikkje er i fase-2-løypa

## Praktisk leseregel

- Om modulen står under "Kan brukast i prosjekt no", kan han brukast i normal fase-2-utvikling
- Om modulen står under "Under arbeid", er han relevant, men ikkje ein del av den smalaste fase-2-løypa
- Om modulen står under "Manglar", er han ikkje del av denne byggfasen enno

## Kva er ferdig i dag

- `std/log.no`
- `std/fil.no`
- `std/cache.no`
- `std/lagring.no`
- `std/innstillingar.no`
- `std/sched.no`
- `std/tråd.no`

## Kva er neste dersom standardbiblioteket skal utvidast

- fleire hjelpefunksjonar for filtrering og oppsummering
- breiare testdekning for ikkje-prioriterte modular
- betre dokumentasjon for modular utanfor fase-2-løypa
