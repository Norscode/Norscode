# Frontend Deploy and Rollback

Frontend deploy og rollback bør være enkle og trygge.

## Mål

- kunne rulle ut nye versjoner kontrollert
- kunne gå tilbake hvis noe går galt
- holde deployflyten forståelig

## Modell

- deploy bruker samme release-prinsipper som resten av prosjektet
- rollback skal være en kjent og dokumentert handling
- statiske assets og appversjon bør passe sammen

## Regler

- deploy skal være reproducerbart
- rollback må ikke kreve spesialtriks
- behold kompatibilitet mellom frontend og backend der det trengs

## Når dette er ferdig

- frontend kan distribueres og reverseres uten drama

Se også [docs/DEPLOYMENT_PLAYBOOK.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/DEPLOYMENT_PLAYBOOK.md).
