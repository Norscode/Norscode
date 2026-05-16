# Frontend API Client

En standard API-klient gjør frontend-til-backend kommunikasjon enkel og konsistent.

## Mål

- samle HTTP-kall ett sted
- gjøre feil og retries forutsigbare
- holde API-oppkalling utenfor komponenter når det passer

## Modell

- API-klienten bygger på `std.http`
- klienten tilbyr små helpers for GET, POST, PUT, PATCH og DELETE
- klienten håndterer JSON, statuskoder og feil på en konsekvent måte

## Regler

- bruk samme klient for hele appen når mulig
- skjul lavnivådetaljer bak små helpers
- la response-feil være lesbare

## Når dette er ferdig

- frontenden kan snakke med backend på en standardisert måte
- nettverkskode er samlet og lettere å teste

Se også [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROADMAP.md).
