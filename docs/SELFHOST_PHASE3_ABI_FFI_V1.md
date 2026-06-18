# Selfhost Fase 3 - ABI og FFI v1

Dette dokumentet samlar den første fase-3-flata for ABI og FFI.

## Formål

- Sikre at offentleg grensesnitt er tydeleg og stabilt
- Gjere FFI-koplinga enkel å lese og vedlikehalde
- Halde intern og offentleg flate skilde

## Stabil flate

Fase 3 byggjer vidare på desse kjende stabile punkta frå fase 2:

- runtime-API v1
- kall-kontraktar for builtin og extern-modular
- migrasjonsbru frå intern bytecode til stabilt API

## ABI-prinsipp

- Funksjonsnamn skal vere dokumenterte før dei vert normative
- Parameterrekkje skal vere stabil for offentleg flate
- Returntype skal vere eksplisitt dokumentert
- Feil og null-returar skal vere tydeleg spesifiserte

## FFI-prinsipp

- FFI skal berre peike på offentleg dokumenterte symbol
- Interne hjelpefunksjonar skal ikkje eksponerast som ABI
- Namnerom skal vere klart skilte frå intern implementasjon

## Kva som skal vere klart

- Kva som er offentleg
- Kva som er intern støtte
- Kva som bryt ABI
- Kva som kan utvidast utan ny versjon

## Forhold til fase 2

- Fase 2 etablerte grunnlaget
- Fase 3 gjer kontrakten vidare og meir eksplisitt
- Ingen ny C- eller Python-basert hovudveg blir lagt inn her
