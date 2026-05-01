# Frontend Cache Model

Cache for lastet data bør være eksplisitt og lett å forstå.

## Mål

- unngå unødvendige API-kall
- gjøre navigasjon raskere
- holde cache atskilt fra ren UI-state

## Modell

- cache ligger i en egen modul eller store
- hver cache-entry har nøkkel, verdi og eventuelt timestamp
- utløp og invalidasjon skal være tydelig

## Regler

- cache det som kan gjenbrukes trygt
- ikke cache sensitiv eller kortlevd informasjon uten grunn
- ha en enkel måte å slette eller oppdatere cache på

## Typiske brukstilfeller

- listevisninger
- brukerprofiler
- konfigurasjon og feature-flags
- data som lastes ofte, men endres sjelden

## Når dette er ferdig

- appen kan gjenbruke lastede data uten å bli forvirrende
- utviklerne vet hva som caches og hvorfor

Se også [docs/FRONTEND_STORE_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_STORE_MODEL.md).
