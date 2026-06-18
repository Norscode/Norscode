# Selfhost Fase 3 - Backend-optimalisering v1

Dette dokumentet bind fase 3 til den eksisterande backend-løypa og gjer vidare optimalisering meir målbar.

## Formål

- Halde backend-løypa enkel å følgje
- Bruke reproduksjon som kontrollpunkt
- Gjere serialisering og lowering lettare å forstå
- Unngå stille fallback når ein optimalisering manglar

## Byggjer på

- [docs/SELFHOST_PHASE2_BACKEND_OPTIMIZATION_PLAN.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE2_BACKEND_OPTIMIZATION_PLAN.md)
- `selfhost/compiler/production_backend.no`
- `selfhost/native_execution/x86_64_backend.no`
- `selfhost/native_execution/elf_layout.no`
- `selfhost/compiler_core/backend_lowering.no`

## Arbeidsområde

### Lowering

- halde IR → backend kort og føreseieleg
- halde ansvar skilja mellom lowering og emitter
- dokumentere input og output per steg

### Instruksjonsval

- halde instruksjonsval deterministisk
- bruke enklaste mogelege instruksjon når det gir same semantikk
- unngå stille fallback på manglande transformasjon

### Serialisering

- halde serialisering eksplisitt og stabil
- skilje format frå innhald
- gjere output lett å samanlikne mellom køyringar

### Reproduserbarheit

- bruke reproducible-build-sjekkar som gate
- la backend-endringar bli målte mot stabil output
- dokumentere avvik før dei blir permanente

## Mål for fase 3

- ikkje introdusere ny kompleksitet utan gevinst
- halde normal kjede utan Python/C
- bruke eksisterande backend-kontraktar som referanse

## Vidare arbeid

- meir presis lowering-dokumentasjon
- endå tydelegare serialiseringsveg
- små, målbare backend-forbetringar når dei gir faktisk gevinst
