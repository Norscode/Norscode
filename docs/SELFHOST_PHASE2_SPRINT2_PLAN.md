# Selfhost Fase 2 - Sprint 2 Plan

Denne sprinten byggjer vidare på backloggen med to konkrete arbeidspakker som kan løysast utan å endre hovudretninga for selfhost.

## Mål for sprinten

- [x] Lage første stabile ABI-skisse for offentlege runtime-eksportar
- [x] Gjere standardbibliotek-status meir synleg og testbar
- [x] Velg konkrete runtime-stubbar som skal bli ekte implementasjonar

Støttedokument:

- [x] [ABI-minimum v1](./SELFHOST_PHASE2_ABI_MINIMUM_V1.md)
- [x] [Runtime-mål](./SELFHOST_PHASE2_RUNTIME_TARGETS.md)
- [x] [Standardbibliotek-løype v1](./SELFHOST_PHASE2_STDLIB_LØYPE_V1.md)

## Arbeidspakke 1: ABI-minimum v1

### Leveransar

- [x] Namnerom for offentlege runtime-funksjonar
- [x] Versjonert signaturliste for eksportar
- [x] Skilje mellom intern og offentleg flate
- [x] Kort dokumentasjon på kva som bryt ABI og kva som ikkje gjer det
- [x] Migrasjonsnotat frå intern bytecode til stabilt API

### Akseptkriterium

- [x] Ei ny person kan lese dokumentet og sjå kva som er stabilt
- [x] Det er klart kva som krev ny versjon
- [x] Det finst eit lite, konkret døme på eksport/bruk
- [x] Det finst ei enkel bru frå intern bytecode til offentleg API

## Arbeidspakke 2: Standardbibliotek-løype v1

### Leveransar

- [x] Velg fem modulane med høgast nytteverdi for normal utvikling
- [x] Gi kvar modul statusen `ferdig`, `pågår` eller `mangler`
- [x] Legg inn minst éin minimal test per modul
- [x] Dokumenter kvar av modulane kan brukast i praksis

### Akseptkriterium

- [x] Statuslista er enkel å skumlese
- [x] Det går an å sjå kva som er klart for bruk
- [x] Det går an å sjå kva som framleis er work in progress

Status:

- [x] Denne arbeidspakka er ferdig

## Runtime-stubbar som kandidatar

- [x] Velg to funksjonar frå `selfhost/runtime/production_runtime.no`
- [x] Skriv ned kvifor dei er prioriterte
- [x] Definer eit minimalt mål for kvar funksjon

## Ferdig når

- [x] Dei to arbeidspakkene har eigne oppgåver og akseptkriterium
- [x] Status er oppdatert i [docs/SELFHOST_HANDLINGSPLAN.md](./SELFHOST_HANDLINGSPLAN.md)
- [x] Sprinten kan visast som ein enkel arbeidsflate utan ekstra forklaring

Status:

- [x] Sprint 2 er ferdig, og standardbibliotek-løypa er konkretisert
