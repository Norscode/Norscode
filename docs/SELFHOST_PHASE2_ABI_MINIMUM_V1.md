# Selfhost Fase 2 - ABI-minimum v1

Dette dokumentet definerer den første stabile ABI-skissa for selfhost-runtime og relaterte eksportar.

## Mål

- Gjere det klart kva som er offentleg flate
- Gjere det klart kva som kan endrast utan versjonsbrot
- Gjere det lett å sjå kva som krev ny ABI-versjon

## ABI-prinsipp

- Offentlege funksjonar skal ha stabile namn og stabile parameterrekkjer
- Ein funksjon som får nye krav skal ikkje endre tyding i stillhet
- Feil skal vere eksplisitte og føreseielege
- Intern flate skal halde seg intern så langt det er praktisk mogleg

## Offentleg namnrom

Første ABI-versjon bruker eit enkelt namnrom for eksportar:

- `runtime.v1.map_memory_page`
- `runtime.v1.protect_memory_region`
- `runtime.v1.allocate_virtual_region`
- `runtime.v1.enqueue_io`
- `runtime.v1.process_io`
- `runtime.v1.schedule_thread`
- `runtime.v1.execute_scheduler_cycle`
- `runtime.v1.acquire_mutex`
- `runtime.v1.release_mutex`
- `runtime.v1.checkpoint_process`
- `runtime.v1.recover_process`

## Stabilitetsreglar

- Parameterrekkja til ein eksport kan ikkje endrast utan ny versjonsflate
- Returtype skal vere stabil for kvar eksport
- Eit nytt valfritt felt kan berre leggjast til dersom gammal bruk framleis fungerer
- Null- og feilmanglar skal vere dokumenterte for kvar funksjon

## Feilkontrakt

- Manglande manager eller runtime skal gi den eksplisitte feilen som alt er brukt i runtime-koden
- Funksjonar som returnerer status skal bruke `sann`/`usann` konsekvent
- Funksjonar som berre muterer tilstand skal ikkje skjule brot bak stille suksess

## Public vs intern

- Offentleg: det som står i namnrommet over
- Intern: hjelpefunksjonar, placeholderar, og eigne runtime-kontrollar som berre støttar den offentlege flata

## Mini-døme

```text
runtime.v1.map_memory_page(manager, address) -> sann/usann
runtime.v1.enqueue_io(runtime, operation) -> sann/usann
runtime.v1.process_io(runtime) -> liste av fullførte operasjonar
```

## Kva krev ny ABI-versjon

- Ny parameterrekkje
- Endra retursemantikk
- Endra feilmodell
- Fjerning av ein eksport
- Tydingsendring i eksisterande namn

## Ferdig når

- ABI-minimum er lista med konkrete namn
- To runtime-funksjonar er valde som første implementasjonsmål
- Sprint 2 kan referere til denne skissa utan ekstra forklaring
