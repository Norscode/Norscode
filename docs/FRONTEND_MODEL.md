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
- en gradvis vei fra HTML mode til template, component, native UI og reactive mode

Native UI er nå en ekte Norscode-modul i [`std/nativeui.no`](/Users/jansteinar/Projects/Norscode/std/nativeui.no) og brukes av `ui-render` som den kanoniske renderer-veien.

For komponent- og slot-modell er [`std/frontend.no`](/Users/jansteinar/Projects/Norscode/std/frontend.no) den anbefalte inngangen, [`std/reactive.no`](/Users/jansteinar/Projects/Norscode/std/reactive.no) er den anbefalte kontraktflaten for state/bindings/events, [`std/islands.no`](/Users/jansteinar/Projects/Norscode/std/islands.no) er kontraktflaten for server/client-splitt, mens [`std/html.no`](/Users/jansteinar/Projects/Norscode/std/html.no) fortsatt er lavnivåflaten.

For lesbare startpunkter og gull-eksempler, se [`docs/FRONTEND_GOLDEN_EXAMPLES.md`](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_GOLDEN_EXAMPLES.md), [`examples/frontend_golden.no`](/Users/jansteinar/Projects/Norscode/examples/frontend_golden.no) og [`examples/native_ui_golden.nui`](/Users/jansteinar/Projects/Norscode/examples/native_ui_golden.nui).

Se også [docs/FRONTEND_MODES.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_MODES.md), [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_ROADMAP.md) og [docs/FRONTEND_NATIVE_UI_ROADMAP.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_NATIVE_UI_ROADMAP.md) for hele planen.
