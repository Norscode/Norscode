# Frontend Navigation Model

Norscode-frontenden bør starte med en enkel, path-basert navigasjonsmodell:

- hver side har en stabil URL
- ruter kan være server-renderte
- direkte lenker skal fungere uten ekstra oppsett
- browseren kan senere få client-side routing oppå samme struktur
- server-drevne fragmenter kan brukes som en mellomvei for delvis rendering

## Hvorfor denne modellen

- den er lett å forstå
- den fungerer godt med `norcode serve`
- den passer både små og store apper
- den gir en naturlig vei videre til client-side routing senere

## Første regler

- bruk lesbare URL-er
- hold navigasjon eksplisitt
- skill mellom sideflyt og intern komponentstate
- behold server fallback som standard
- bruk fragmenter når du vil dele opp renderingen uten å introdusere en klientruntime

## Praktisk konsekvens

Frontend-appens første navigasjon bør kunne beskrives som:

- `/` for hjem
- `/sok` for søk med query-parametre
- `/poster` for paginerte listevisninger
- `/ruter` for en samleside over de viktigste URL-ene
- `/side` for egne sider
- `/kategori/objekt` for dypere strukturer når det trengs

## Neste steg

- [`FRONTEND_MODEL`](./FRONTEND_MODEL.md)
- [`FRONTEND_FRAGMENT_MODEL`](./FRONTEND_FRAGMENT_MODEL.md)
- [`FRONTEND_LEARNING_PATH`](./FRONTEND_LEARNING_PATH.md)
