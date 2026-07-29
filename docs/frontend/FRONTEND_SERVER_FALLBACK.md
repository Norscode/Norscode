# Frontend Server Fallback

Direkte lenker og refresh skal alltid fungere via server fallback.

## Mål

- sikre at browseren kan åpne enhver gyldig side direkte
- gjøre routing robust ved refresh
- beholde path som sannhet

## Kontrakt

- serveren må kunne levere samme side for direkte tilgang og navigering
- ukjente ruter bør kunne gi en ryddig 404
- fallback må ikke bryte canonical URL-er

## Regler

- bruk server-rendering som sikker fallback
- ikke krev client-only routing for grunnleggende tilgang
- la appen gi samme opplevelse ved første load og senere navigering

## Praktisk konsekvens

- `/brukere/42` kan åpnes direkte
- `/ruter` kan åpnes direkte og vise en samlet ruteoversikt
- `/sok?q=server` kan åpnes direkte og fortsatt gi samme søkeflyt
- `/poster?side=2` kan åpnes direkte og fortsatt gi samme paginerte visning
- refresh på en side skal fungere
- delte lenker skal være stabile
- fragmenter kan brukes når bare deler av siden skal server-rendres separat

## Når dette er ferdig

- brukeren kan dele en URL og komme tilbake til samme visning
- appen fungerer også uten client-side state i minnet

## Se også

- [`FRONTEND_NAVIGATION_MODEL`](./FRONTEND_NAVIGATION_MODEL.md)
- [`FRONTEND_CLIENT_ROUTING`](./FRONTEND_CLIENT_ROUTING.md)
- [`FRONTEND_FRAGMENT_MODEL`](./FRONTEND_FRAGMENT_MODEL.md)
- [`FRONTEND_LEARNING_PATH`](./FRONTEND_LEARNING_PATH.md)
