# Frontend Server Validation

Servervalidering skal være autoritativ og gi tydelige feilmeldinger.

## Mål

- sikre korrekt data på serveren
- gi brukeren forståelige feil
- matche klientvalideringen så godt som mulig

## Modell

- serveren validerer alltid ved submit
- feilmeldinger returneres til skjemaet
- feltfeil og skjema-feil skilles tydelig

## Regler

- ikke stol på klientvalidering alene
- gi konkrete meldinger per felt
- vis om feilen gjelder ett felt eller hele skjemaet
- la serverfeil være lett å rendre tilbake i UI

## Når dette er ferdig

- skjemaet kan vise valideringsfeil uten å miste brukerdata
- appen er trygg selv når klienten er manipulert

Se også [docs/FRONTEND_CLIENT_VALIDATION.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_CLIENT_VALIDATION.md).
