# Frontend Network Errors

Nettverksfeil bør være tydelige og håndterbare i frontend.

## Mål

- skille mellom transient og permanent feil
- gi brukeren noe konkret å gjøre videre
- unngå generiske feilmeldinger

## Modell

- vis statuskode eller kort forklaring
- gi retry der det passer
- behold brukerens data ved feil
- logg tekniske detaljer separat

## Regler

- nettverksfeil skal ikke se ut som valideringsfeil
- vis om problemet kan prøves på nytt
- gi rolig og forståelig feedback

## Når dette er ferdig

- nettverksfeil føles håndterbare
- appen mister ikke brukerens arbeid ved et midlertidig avbrudd

Se også [docs/FRONTEND_RETRY_POLICY.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_RETRY_POLICY.md).
