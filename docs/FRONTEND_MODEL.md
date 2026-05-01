# Frontend Model

Norscode sin første frontend-versjon bør være:

- server-renderte sider som standard
- komponentbasert templating for gjenbruk
- små interaktive islands der det faktisk trengs

Dette gir en smal og stabil grunnmur:

- vi gjenbruker `std.web` og `norcode serve`
- vi slipper å bygge en full SPA-stack først
- vi får tydelige sider, layout og statiske ressurser uten tung kompleksitet

## Hvorfor denne modellen

### 1. Den passer dagens språk og runtime

Norscode har allerede:

- request/response
- routing
- templating-nære string helpers
- cookies, CSRF, auth og OpenAPI
- serveradapter og deployflyt

Det gjør server-renderte sider til den naturlige første modellen.

### 2. Den holder frontend enkel å forstå

En side bør være:

- en handler som returnerer HTML
- satt sammen av små komponenter
- med én tydelig layout

Det gjør det lett å lese, teste og feilsøke.

### 3. Den gir en god vei videre

Når grunnmuren er på plass, kan vi senere utvide med:

- client-side routing
- state/store
- skjemaer og validering
- reaktive widgets
- hydrering eller islands

## Første prinsipper

- Bruk HTML som standard output.
- Bruk små funksjoner for komponenter.
- Hold layout og innhold adskilt.
- La statiske ressurser være eksplisitte.
- Ikke introduser mer frontend-kompleksitet enn appen trenger.

## Praktisk konsekvens

Frontend-roadmapens første etappe blir derfor å etablere:

- prosjektstruktur
- side-/layout-kontrakt
- statiske assets
- enkel dev-flyt for frontend-ressurser

Se også [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROADMAP.md) for hele planen.
