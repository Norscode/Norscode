# Frontend Roadmap

Dette er en egen roadmap for en full frontend-flate bygget med Norscode.
Den tar utgangspunkt i det som allerede finnes for web, HTTP, auth, docs og serverkjøring,
men flytter fokus over til brukeropplevelse i browseren.

## Mål

- gjøre det lett å bygge en ekte frontend-app i Norscode
- beholde Norscode-stil, ikke kopiere et eksisterende frontend-rammeverk 1:1
- starte med en enkel og stabil grunnmur, og bygge videre i små, testbare steg

## Hva som allerede finnes

- [x] `std.web` for request/response, routing, guards, middleware og OpenAPI
- [x] `std.http` for klient-HTTP
- [x] async/await-grunnmur
- [x] JSON, filer, cache, logging, metrics og tracing
- [x] `norcode serve` for lokal webserver og dev-flyt
- [x] CORS, cookies, CSRF og auth-hjelpere
- [x] deploy-, container- og observability-støtte

## Etappe 1: Frontend-grunnmur

Hvorfor:
Før vi kan bygge full frontend, må vi ha en tydelig måte å representere UI, sider og statisk innhold på.

Leveranse:

- [x] velge første frontend-modell: server-renderte sider, komponentbasert templating eller hybrid
- [x] definere prosjektstruktur for frontend-app
- [x] lage enkel side-/layout-kontrakt
- [x] etablere statisk asset-flate for CSS, bilder og ikoner
- [x] lage enkel dev-flyt for frontend-ressurser

Ferdig når:

- [x] en frontend-app kan starte opp med en tydelig side og felles layout
- [x] statiske ressurser kan leveres uten manuelt oppsett per app
  - [x] første app kan utvikles med `norcode serve` og `frontend/assets/`

## Etappe 2: Komponenter og templating

Hvorfor:
En frontend trenger gjenbrukbare byggesteiner for skjermbilder, skjemaer og layout.

Leveranse:

- [x] komponentmodell for UI-biter
- [x] slots eller barn-innhold for komposisjon
- [x] props/parametere for komponenter
- [x] liste- og tabellkomponenter
- [x] enkel måte å gjenbruke layout og delvise visninger

Ferdig når:

- [x] en side kan bygges av små gjenbrukbare komponenter
- [x] layout og sideinnhold kan kombineres uten duplisering

## Etappe 3: Navigasjon og sideflyt

Hvorfor:
Frontend må kunne bevege seg mellom sider og holde orden på state mellom views.

Leveranse:

- [x] velge navigasjonsmodell: path-baserte sider med server fallback
- [x] client-side routing
- [x] route-parametre og query-parametre i browseren
- [x] aktiv lenke-/menytilstand
- [x] fallback til server-side routing for direkte lenker
- [x] enkel side- og modulstruktur

Ferdig når:

- [x] brukeren kan navigere mellom sider uten at appen mister struktur

## Etappe 4: State og dataflyt

Hvorfor:
En full frontend trenger en tydelig måte å håndtere state, cache og sideeffekter på.

Leveranse:

- [x] local state for komponenter
- [x] delt app-state eller store
- [x] cache for lastet data
- [x] optimistic updates der det passer
- [x] feil- og loading-tilstander som standard

Ferdig når:

- [x] appen kan håndtere skjemaer, lister og oppdateringer uten ad hoc-løsninger

## Etappe 5: Skjemaer og validering

Hvorfor:
De fleste seriøse frontends bruker mye tid på skjemaer og input.

Leveranse:

- [x] input-komponenter
- [x] skjema-binding
- [x] klientvalidering
- [x] servervalidering med tydelige feilmeldinger
- [x] støtte for async submit og disabling under sending

Ferdig når:

- [x] brukeren får tydelig hjelp før og etter innsending

## Etappe 6: Styling og designsystem

Hvorfor:
Frontend må se ut som ett produkt, ikke som mange tilfeldige sider.

Leveranse:

- [x] design tokens for farger, spacing og typografi
- [x] komponentbibliotek for knapper, inputs, kort og dialoger
- [x] mørk/lys modus hvis det gir verdi
- [x] responsive layouts
- [x] tilgjengelighet som standard

Ferdig når:

- [x] appen kan holdes visuelt konsistent på tvers av skjermstørrelser

## Etappe 7: API-integrasjon og browserflyt

Hvorfor:
En moderne frontend lever sjelden alene, den snakker med backend hele tiden.

Leveranse:

- [x] standard API-klient for `std.http`
- [x] auth token/cookie-håndtering i browser
- [x] retry og backoff der det passer
- [x] SSE/WebSocket-vurdering hvis appen trenger sanntidsdata
- [x] tydelig feilbehandling for nettverksfeil

Ferdig når:

- [x] frontenden kan hente, sende og oppdatere data mot backend på en robust måte

## Etappe 8: Testing og kvalitet

Hvorfor:
Frontend uten gode tester og visuell kontroll blir fort skjør.

Leveranse:

- [x] komponenttester
- [x] side-/route-tester
- [x] browser-E2E
- [x] visuell regresjon der det gir verdi
- [x] accessibility-sjekker

Ferdig når:

- [x] de viktigste brukerflytene kan verifiseres automatisk

## Etappe 9: Bygg og distribusjon

Hvorfor:
En frontend er ikke ferdig før den er lett å bygge, cache og sende ut til brukere.

Leveranse:

- [x] asset-bundling eller enkel statisk publisering
- [x] cache-hoder for statiske filer
- [x] container- eller CDN-flyt
- [x] miljøstyring for API-endepunkt og feature flags
- [x] enkel deploy-/rollback-flyt

Ferdig når:

- [x] frontend kan distribueres like ryddig som backend

## Anbefalt rekkefølge

- [x] Frontend-grunnmur
- [x] Komponenter og templating
- [x] Navigasjon og sideflyt
- [x] State og dataflyt
- [x] Skjemaer og validering
- [x] Styling og designsystem
- [x] API-integrasjon og browserflyt
- [x] Testing og kvalitet
- [x] Bygg og distribusjon

## Kortversjon

Frontend-sporet er ferdig når:

- [x] det er lett å bygge en app med tydelige sider og komponenter
- [x] state, routing og skjemaer henger sammen
- [x] design og tilgjengelighet er standard, ikke ettertanke
- [x] browserflyten kan testes og distribueres trygt
