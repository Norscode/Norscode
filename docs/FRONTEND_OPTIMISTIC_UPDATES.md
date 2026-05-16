# Frontend Optimistic Updates

Optimistic updates betyr at UI oppdateres før serveren har svart, når det er trygt.

## Mål

- gjøre appen rask og responsiv
- redusere ventetid i brukerflyt
- håndtere feil uten å miste kontroll

## Modell

- oppdater UI umiddelbart
- send handlingen til serveren parallelt
- rull tilbake hvis serveren feiler

## Regler

- bruk optimistic updates bare når feil kan håndteres ryddig
- vis tydelig at en endring er uavklart
- hold rollback-logikken enkel

## Eksempler

- favorittmerking
- toggles og små preferansevalg
- enkle listeoppdateringer

## Når dette er ferdig

- UI føles raskere uten å bli uforutsigbart
- brukeren kan se hva som skjer mens serveren jobber

Se også [docs/FRONTEND_CACHE_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_CACHE_MODEL.md).
