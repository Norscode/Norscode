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

## JS-fri mellomvei

Før Norscode eventuelt får en klientruntime, kan vi bruke server-drevne fragmenter som en tydelig del av modellen:

- serveren kan returnere et fragment i stedet for en full side
- samme komponenter kan brukes i fullside- og fragment-svar
- dette holder browseren enkel og lar oss forbedre dataflyt uten å innføre JS først

## Når dette er ferdig

- appen kan bruke klientnavigasjon uten å miste server-støtte
- routes oppfører seg likt på første last og senere navigasjon

Se også [docs/FRONTEND_NAVIGATION_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_NAVIGATION_MODEL.md) og [docs/FRONTEND_FRAGMENT_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_FRAGMENT_MODEL.md).
