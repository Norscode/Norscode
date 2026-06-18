# Selfhost Fase 2 - Backend-optimalisering

Dette dokumentet skisserer neste steg for å gjere backend-løypa meir effektiv og meir robust.

## Kva vi byggjer vidare på

- `selfhost/compiler/production_backend.no`
- `selfhost/native_execution/x86_64_backend.no`
- `selfhost/native_execution/phase1_native_backend_complete.no`
- `selfhost/native_execution/elf_layout.no`
- `selfhost/compiler_core/backend_lowering.no`

## Mål

- redusere unødvendige mellomsteg i backend-flata
- gjere generering og serialisering meir føreseieleg
- halde reproduksjon og determinisme tydeleg
- styrke det som allereie fungerer før vi utvidar

## Arbeidsområde

### 1. Lowering

- halde IR → backend-løypa kort
- fjerne duplisert ansvar mellom lowering og backend-emitter
- dokumentere kva som er input og output per steg

Minimum:

- kvar backend-funksjon skal ha eitt klart ansvar
- lowering-feil skal vere lett å spore tilbake til input

### 2. Instruksjonsval

- velje enklaste mogelege instruksjon for kvar operasjon
- halde registerbruk stabil og leseleg
- unngå stille fallback når ein transformasjon manglar

Minimum:

- ikkje legg til ny kompleksitet utan at det gir målbar gevinst
- instruksjonsval skal vere deterministisk

### 3. Serialisering

- hald ELF-/binary-serialisering enkel og eksplisitt
- skil klart mellom format, innhald og skriveveg
- gjer det lett å samanlikne output mellom køyringar

Minimum:

- same input skal gi same serialiserte struktur
- serialiseringsfeil skal vere direkte lesbare

### 4. Reproduserbarheit

- bruk eksisterande reproducible-build-sjekkar som gate
- la backend-endringar bli målt mot stabil output
- dokumenter avvik før dei blir permanente

Minimum:

- `verify_reproducible_binary` og liknande kontrollar skal vere lette å bruke i CI

## Prioriteringsrekkefølgje

1. Lowering
2. Reproduserbarheit
3. Serialisering
4. Instruksjonsval og små optimaliseringar

## Akseptkriterium

- planen skal vere lesbar som arbeidsreferanse
- planen skal peike på dei faktiske backend-modulane
- planen skal gjere det klart kva som kan optimaliserast utan å bryte normal kjede
