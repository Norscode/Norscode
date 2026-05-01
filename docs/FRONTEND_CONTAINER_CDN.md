# Frontend Container and CDN Flow

Frontend kan distribueres via container eller CDN avhengig av miljø.

## Mål

- gjøre deploy enkelt
- støtte både små og store installasjoner
- holde statiske filer lett tilgjengelige

## Modell

- container brukes for app-server og evt. SSR
- CDN eller statisk hosting brukes for assets når det passer
- miljøet bestemmer endepunkt og distribusjonsmåte

## Regler

- bygg og deploy skal være reproducerbart
- statiske filer bør være enkle å flytte til CDN senere
- ikke lås frontend til én deploymodell

## Når dette er ferdig

- frontend kan kjøres i container eller publiseres som statiske assets

Se også [docs/DEPLOYMENT_PLAYBOOK.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/DEPLOYMENT_PLAYBOOK.md) og [docs/CONTAINER.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/CONTAINER.md).
