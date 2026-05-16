# Frontend Navigation Model

Norscode-frontenden bør starte med en enkel, path-basert navigasjonsmodell:

- hver side har en stabil URL
- ruter kan være server-renderte
- direkte lenker skal fungere uten ekstra oppsett
- browseren kan senere få client-side routing oppå samme struktur

## Hvorfor denne modellen

- den er lett å forstå
- den fungerer godt med `norcode serve`
- den passer både små og store apps
- den gir en naturlig vei videre til client-side routing senere

## Første regler

- bruk lesbare URL-er
- hold navigasjon eksplisitt
- skill mellom sideflyt og intern komponentstate
- behold server fallback som standard

## Praktisk konsekvens

Frontend-appens første navigasjon bør kunne beskrives som:

- `/` for hjem
- `/side` for egne sider
- `/kategori/objekt` for dypere strukturer når det trengs

## Neste steg

Se [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROADMAP.md) for resten av navigasjon-etappen.
