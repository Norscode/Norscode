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
- `std/asynk.no` - worker-konfig, oppgåvekø, bakgrunnsjobbar, timeout/retry og streaming-respons
- `std/stdlib_status.no` - maskinlesbar statusmatrise for stabil/eksperimentell/stub
- Python-liknande standardmodular kan brukast som ergonomisk inspirasjon, men aktiv stdlib skal ikkje starte `python3`
- praktisk brukscase-dekning ligg i [docs/SELFHOST_PHASE2_STDLIB_USECASES.md](./SELFHOST_PHASE2_STDLIB_USECASES.md)

## Under arbeid

- lengre regresjonssett per modulområde

## Manglar

- full økosystem-paritet med Python-pakkar og tredjepartsbibliotek
- eit endå breiare testnett for moduler som ikkje er i fase-2-løypa

## Praktisk leseregel

- Om modulen står under "Kan brukast i prosjekt no", kan han brukast i normal fase-2-utvikling
- Om modulen står under "Under arbeid", er han relevant, men ikkje ein del av den smalaste fase-2-løypa
- Om modulen står under "Manglar", er han utanfor denne byggfasen

## Kva er ferdig i dag

- `std/log.no`
- `std/fil.no`
- `std/cache.no`
- `std/lagring.no`
- `std/innstillingar.no`
- `std/sched.no`
- `std/tråd.no`
- `std/asynk.no`
- `std/stdlib_status.no`

## Kva er neste dersom standardbiblioteket skal utvidast

- fleire hjelpefunksjonar for filtrering og oppsummering
- breiare testdekning for ikkje-prioriterte modular
- betre dokumentasjon for modular utanfor fase-2-løypa
