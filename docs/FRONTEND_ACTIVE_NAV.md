# Frontend Active Navigation State

Aktiv lenke- og menytilstand gjør navigasjonen tydelig for brukeren.

## Mål

- vise hvor brukeren er
- gjøre menyen enklere å lese
- holde aktiv tilstand knyttet til URL

## Kontrakt

- aktiv tilstand bestemmes av nåværende path
- aktiv meny skal følge route-match, ikke lokal tilfeldighet
- både lenker og menypunkter kan markeres som aktive

## Regler

- bruk `active`-klasse eller tilsvarende som standard
- ikke lag separat state hvis URL-en allerede forteller sannheten
- hold logikken enkel og deterministisk

## Eksempler

- `/brukere` markerer `Brukere`
- `/innstillinger/profil` markerer `Innstillinger`
- query-parametre skal bare påvirke aktiv tilstand hvis de faktisk endrer sidevalg

## Når dette er ferdig

- menyen viser tydelig valgt side
- brukeren ser hvor i appen de er

Se også:

- [docs/FRONTEND_NAVIGATION_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_NAVIGATION_MODEL.md)
- [docs/FRONTEND_ROUTE_PARAMS.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROUTE_PARAMS.md)
