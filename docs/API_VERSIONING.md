# API Versioning

Norscode API-kontrakter versjoneres primært med URL-prefiks for major-versjoner:

- `/api/v1/...`
- `/api/v2/...`

Dette gjør det enkelt å kjøre flere kontraktsversjoner parallelt mens klienter migrerer.

## Anbefalt praksis

- Hold breaking changes i en ny major-versjon.
- Behold gamle endepunkter til klientene er migrert.
- Generer egen OpenAPI for hver versjon.
- Sett `info.version` i OpenAPI til kontraktsversjonen, ikke bare pakkens releaseversjon.
- Bruk subroutere eller prefiks i `web.route(...)` for å holde v1 og v2 separat.

## Eksempel

Se [examples/web_api_versioning.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_api_versioning.no).

