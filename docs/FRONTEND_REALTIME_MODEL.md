# Frontend Realtime Model

SSE eller WebSocket bør bare brukes når appen faktisk trenger sanntidsdata.

## Mål

- holde API-integrasjon enkel
- unngå overkompliserte realtime-løsninger
- ha et tydelig valg når live-oppdateringer trengs

## Modell

- bruk polling eller vanlige API-kall som standard
- vurder SSE for enveis live-oppdateringer
- vurder WebSocket når det er behov for toveis kommunikasjon

## Regler

- ikke start med realtime hvis vanlige kall holder
- la transportvalg styres av behov, ikke trend
- hold reconnect og feilhåndtering tydelig

## Når dette er ferdig

- appen vet når den skal bruke vanlig polling, SSE eller WebSocket

Se også [docs/FRONTEND_API_CLIENT.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_API_CLIENT.md).
