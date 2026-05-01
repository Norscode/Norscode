# Frontend Server Fallback

Direkte lenker og refresh skal alltid fungere via server fallback.

## Mål

- sikre at browseren kan åpne enhver gyldig side direkte
- gjøre routing robust ved refresh
- beholde path som sannhet

## Kontrakt

- serveren må kunne levere samme side for direkte tilgang og navigering
- unknown routes bør kunne gi ryddig 404
- fallback må ikke bryte canonical URL-er

## Regler

- bruk server-rendering som sikker fallback
- ikke krev client-only routing for grunnleggende tilgang
- la appen gi samme opplevelse ved første load og senere navigering

## Praktisk konsekvens

- `/brukere/42` kan åpnes direkte
- refresh på en side skal fungere
- delte lenker skal være stabile

## Når dette er ferdig

- brukeren kan dele en URL og komme tilbake til samme visning
- appen fungerer også uten client-side state i minnet

Se også:

- [docs/FRONTEND_NAVIGATION_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_NAVIGATION_MODEL.md)
- [docs/FRONTEND_CLIENT_ROUTING.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_CLIENT_ROUTING.md)
