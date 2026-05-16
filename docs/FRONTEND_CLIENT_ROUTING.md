# Frontend Client-Side Routing

Client-side routing i Norscode bør komme som et lag oppå den eksisterende path-baserte modellen.

## Mål

- beholde server fallback som standard
- gjøre navigasjon raskere der det gir verdi
- holde direkte lenker og refresh trygt

## Modell

- serveren eier canonical routes
- browseren kan oppdatere visning uten full reload
- route-definisjoner skal være delte mellom server og klient der det er mulig

## Første regler

- ikke kreve client-side routing for at appen skal fungere
- behold URL som sannhet
- synkroniser navigasjon med browser history
- bruk enkel route-map før mer avansert routerlogikk

## Hva vi ønsker i første versjon

- lenker som kan interceptes av klienten
- sidevisning som kan skiftes uten full reload
- server fallback ved refresh og direkte entry

## Når dette er ferdig

- appen kan bruke klientnavigasjon uten å miste server-støtte
- routes oppfører seg likt på første last og senere navigasjon

Se også [docs/FRONTEND_NAVIGATION_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_NAVIGATION_MODEL.md).
