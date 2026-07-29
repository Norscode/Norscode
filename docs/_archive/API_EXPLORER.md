# API Explorer (Interactive API docs)

Denne siden tilbyr en enkel, lokal, interaktiv API-oplevelse:

- Last inn et OpenAPI 3-skjema fra en URL (for eksempel `/openapi.json`)
- Se og velg registrerte operasjoner (metode + path)
- Bygg og send forespørsler direkte fra nettleseren
- Se headers, body og respons i sanntid
- Kopier automatisk `curl`-kommando for hver forespørsel

## Hvordan bruke

- Åpne `docs/api-explorer/index.html`
- Sett `API base URL` og `OpenAPI URL`
- Klikk **Hent OpenAPI**
- Velg en rute fra venstresiden for å fylles ut automatisk
- Juster headers/body og klikk **Send request**

Når `OpenAPI URL` ikke kan hentes (for eksempel ved lokal første start), faller siden automatisk tilbake til en innebygd demo-spesifikasjon med hjelpetabeller for et lite helpdesk-sett.

## Notater

- Dette er en ren frontend-løsning (HTML/JS/CSS), ingen Python/C.
- Bruk en ekte backend med CORS-tilgang når siden kjøres fra egen origin.
