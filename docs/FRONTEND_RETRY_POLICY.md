# Frontend Retry Policy

Retry og backoff bør være kontrollert og enkel.

## Mål

- tåle korte nettverksfeil
- unngå å overbelaste backend
- gjøre klienten mer robust

## Modell

- retries brukes bare når det er trygt
- backoff øker mellom forsøk
- feil som ikke er transient bør ikke retries blindt

## Regler

- retry GET oftere enn muterende kall
- ikke skjul permanente feil bak retry-loops
- gi brukeren tydelig feedback når det feiler

## Når dette er ferdig

- frontenden håndterer korte nettverksglitcher bedre
- API-kall blir mer robust uten å bli aggressiv

Se også [docs/FRONTEND_API_CLIENT.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_API_CLIENT.md).
