# Frontend Environment and Feature Flags

Frontend bør kunne styres av miljøvariabler og feature flags.

## Mål

- gjøre deploy til ulike miljøer enkel
- styre backend-endepunkt og funksjoner uten kodeendring
- skille mellom test, staging og produksjon

## Modell

- miljøvariabler for API-endepunkt og base-URL
- feature flags for nye funksjoner
- tydelige defaults

## Regler

- ikke hardkod miljøspesifikke verdier
- hold flagg lesbare og få
- dokumenter hvilke flagg som finnes

## Når dette er ferdig

- frontend kan kjøre i flere miljøer uten kodeendring

Se også [docs/FRONTEND_API_CLIENT.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_API_CLIENT.md).
