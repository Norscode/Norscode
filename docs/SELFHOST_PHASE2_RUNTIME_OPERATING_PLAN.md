# Selfhost Fase 2 - Production runtime-mål

Dette dokumentet samlar den operative planen for runtime-arbeidet i fase 2.

## Mål

- få ein meir tydeleg produksjonsflate for runtime
- redusere plass for uavklarte kontraktar og historiske mock-stiar
- gjere det lett å sjå kva som er stabilt, kva som er på veg, og kva som er intern støtte

## Område

### 1. Minne

Målet er å halde minneoperasjonar enkle og føreseielege.

- `map_memory_page` skal ha eksplisitt feil ved manglande manager
- `protect_memory_region` skal feile tydeleg ved ugyldig region
- `allocate_virtual_region` skal ikkje skjule null- eller duplikattilstand

Minimum:

- éin eksplisitt kontrakt per offentleg minnefunksjon
- inga stille suksess ved manglande runtime-objekt

### 2. Sandbox

Målet er å halde runtime-grensene tydelege.

- interne kontrollar skal vere interne
- offentleg flate skal berre eksponere versjonerte funksjonar
- avgrensingar skal vere lette å forklare i CI og dokumentasjon

Minimum:

- ingen nye C- eller Python-omvegar i normal kjede
- kvart nytt runtime-signal skal ha stabil retursemantikk

### 3. I/O

Målet er ein liten og føreseieleg async-veg.

- `enqueue_io` skal vere enkel å kalle og enkel å feile på
- `process_io` skal returnere fullførte operasjonar utan stille dropp
- kø-oppførsel skal vere lett å teste i smoke-løp

Minimum:

- ikkje tap av operasjonar utan eksplisitt feil
- køstatus skal kunne lesast i statusflata

### 4. Scheduler

Målet er å halde planlegginga stabil og enkel å inspisere.

- `schedule_thread` skal vere ein rein kø-operasjon
- `execute_scheduler_cycle` skal gje eit lesbart resultat
- status skal vise om køen er tom, aktiv eller blokkert

Minimum:

- éin kontrollert syklus per test
- inga stille overskriving av køtilstand

## Prioriteringsrekkefølgje

1. Minnekontraktar
2. I/O-kø og prosessering
3. Scheduler-syklus
4. Sandbox-avgrensingar og rydding av uavklart kontrakttekst

## Akseptkriterium

- ein lesar skal kunne sjå kva runtime-delar som er operative mål
- ein lesar skal kunne sjå kva som er første implementasjonsrekkefølgje
- planen skal vere kort nok til å brukast som arbeidsreferanse
