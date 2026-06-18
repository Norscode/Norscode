# Selfhost Fase 2 - Migrasjonsnotat frå intern bytecode til stabilt API

Dette notatet forklarer korleis vi går frå intern bytecode-orientert runtime til ei meir stabil offentleg API-flate.

## Kvifor dette notatet finst

Fase 2 har no ein ABI-minimumsskisse, runtime-API, kall-kontraktar og CI-status.
Det som manglar er ei kort bru som forklarer korleis vi brukar desse delane til å migrere utan å bryte normal løype.

## Utgangspunkt

- Intern bytecode- og runtime-logikk finst allereie i selfhost-laget
- Offentleg flate er no versjonert som `runtime.v1.*`
- Standardbiblioteket har statusflater og smoke-testar
- CI kan vise ein enkel fase-2-gate

## Migrasjonsmål

- halde intern bytecode intern så lenge som mogleg
- eksponere berre stabile operasjonar gjennom `runtime.v1`
- gjere overgangar lesbare i dokumentasjon og CI
- unngå å flytte meir enn éi grense om gongen

## Migrasjonsprinsipp

### 1. Behald intern bytecode som implementasjonsdetalj

Intern bytecode skal framleis vere den mekaniske kjernen for køyring der det er praktisk.
Det betyr at API-endringar ikkje skal krevje at heile bytecode-laget blir offentleg.

### 2. Løft berre stabile overgangar til offentleg API

Når ein funksjon er nyttig nok til å kallast frå fleire delar av systemet, skal han flyttast inn i `runtime.v1.*`.
Funksjonar som framleis endrar seg ofte, skal bli verande intern hjelpelogikk.

### 3. Bruk ein-til-ein-mapping der det er mogleg

Ein stabil API-funksjon bør ha eitt tydeleg ansvar.
Døme:

- `runtime.v1.enqueue_io` -> legg inn ny I/O-hending
- `runtime.v1.process_io` -> prosesser ventande I/O og returner resultat
- `runtime.v1.schedule_thread` -> legg tråd i kø
- `runtime.v1.execute_scheduler_cycle` -> køyr éin planleggjar-syklus

### 4. Flytt feilmønster før implementasjonsdetalj

Når ein intern funksjon blir offentleg, må feilmelding og returtype vere stabil først.
Implementasjonen kan halde fram å utvikle seg så lenge kontrakten er den same.

## Praktisk migrasjon

### Steg A: Kartlegg intern funksjon

- finn kva intern bytecode- eller runtimefunksjon som gjer jobben
- noter kva som er input, output og feil
- ver sikker på at funksjonen er liten nok til å vere stabil

### Steg B: Definer offentleg kontrakt

- legg funksjonen inn i ABI-minimum
- dokumenter returtype og null-/feilreglar
- knyt han til eit konkret namnrom i `runtime.v1`

### Steg C: Oppdater standardbibliotek og smoke-testar

- bruk den nye offentlege flata frå minst éin std-modul eller test
- lag ein liten smoke-test som viser at funksjonen verkar
- registrer resultatet i fase-2-status og CI-status

### Steg D: Reduser intern kall-overflate

- la intern bytecode bruke den offentlege flata, ikkje omvendt, når det er naturleg
- fjern dupliserte hjelpefunksjonar gradvis
- hald berre igjen intern logikk som framleis trengs for ytelse eller bootstrap

## Kva som ikkje skal skje

- intern bytecode skal ikkje bli flytta ut som ny offentleg normalveg
- C- eller Python-baserte omvegar skal ikkje inn i normal kjede
- nye API skal ikkje introduserast utan versjonsflate
- feilmodell skal ikkje endrast stille

## Kva dette betyr for fase 2

Dette notatet gjer fase 2 meir retningsstyrt:

- ABI-minimum seier kva som er stabilt
- runtime-API seier kva som er offentleg
- CI-status seier om vi faktisk held løypa
- dette notatet seier korleis vi går vidare utan å bryte normal bruk

## Minimumskrav for ferdig migrasjonsnotat

- ein lesar skal skjøne kva som er intern bytecode og kva som er offentleg API
- ein lesar skal sjå kva som er første migrasjonspunkt
- notatet skal kunne brukast som arbeidsflate saman med ABI-minimum og CI-status
