# Selfhost Fase 2 - Runtime API v1

Dette dokumentet kartlegg dei første offentlege runtime-API-ane som skal vere stabile i fase 2.

## Stabil flate

### Virtual memory

- `runtime.v1.map_memory_page(manager, address) -> sann/usann`
- `runtime.v1.protect_memory_region(manager, region) -> sann/usann`
- `runtime.v1.allocate_virtual_region(manager, region) -> sann/usann`

### Async I/O

- `runtime.v1.enqueue_io(runtime, operation) -> sann/usann`
- `runtime.v1.process_io(runtime) -> liste`

### Scheduler

- `runtime.v1.schedule_thread(scheduler, thread) -> sann/usann`
- `runtime.v1.execute_scheduler_cycle(scheduler) -> liste`

### Thread sync

- `runtime.v1.acquire_mutex(sync, mutex, thread) -> sann/usann`
- `runtime.v1.release_mutex(sync, mutex) -> sann/usann`

### Crash recovery

- `runtime.v1.checkpoint_process(recovery, process) -> sann/usann`
- `runtime.v1.recover_process(recovery, process) -> sann/usann`

### Kernel mediation

- `runtime.v1.mediate_kernel_request(mediator, syscall) -> sann/usann`
- `runtime.v1.deny_kernel_request(mediator, syscall) -> sann/usann`

## Praktisk status

- [x] Dei fleste funksjonane er definert i `selfhost/runtime/production_runtime.no`
- [x] Funksjonane bruker eksplisitte feilmeldingar på manglande runtime-objekt
- [x] Funksjonane returnerer status der det er naturleg
- [x] Namnerommet er no dokumentert som stabil ABI-flate for fase 2
- [x] Endeleg ABI-dokumentasjon for eksportformat er ferdig for fase 2

## Intern flate

Dette er ikkje del av den offentlege runtime-flata:

- hjelpefunksjonar som `liste_inneheld`
- hjelpefunksjonar som `legg_til_unik`
- interne status-/valideringsfunksjonar i std-modular

## Kva som må haldast stabilt

- Funksjonsnamn
- Parameterrekkje
- Returntype
- Eksplisitte feilmeldingar

## Kva som kan utviklast vidare

- Betre feilmeldingar
- Meir presis statusflate
- Eventuelle nye v1-funksjonar som ikkje bryt signaturen
