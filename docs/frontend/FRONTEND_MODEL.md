# Frontend Model

Norscodes første frontendmodell bør være enkel, stabil og lett å utvide.
Start med server-renderte sider, legg på komponenter der gjenbruk hjelper, og bruk islands eller fragmenter først når det faktisk gir mening.

## Modell

- server-renderte sider som standard
- komponentbasert templating for gjenbruk
- små interaktive islands der det trengs
- server-drevne fragmenter når en del av siden bør rendres separat

Denne modellen gir en smal og kontrollert grunnmur:

- vi gjenbruker `std.web` og `norcode serve`
- vi slipper å bygge en full SPA-stack først
- vi får tydelige sider, layout og statiske ressurser uten tung kompleksitet

## Hvorfor denne modellen

### Passer dagens språk og runtime

Norscode har allerede request/response, routing, templating-nære string helpers, cookies, CSRF, auth, OpenAPI, serveradapter og deployflyt.
Det gjør server-renderte sider til den naturlige første frontendmodellen.

### Holder frontend enkel å forstå

En side bør være en handler som returnerer HTML, satt sammen av små komponenter, med én tydelig layout.
Det gjør det lettere å lese, teste og feilsøke.

### Gir en god vei videre

Når grunnmuren er på plass, kan vi senere utvide med client-side routing, state/store, skjemaer, validering, reaktive widgets og hydrering.

## Første prinsipper

- Bruk HTML som standard output.
- Bruk små funksjoner for komponenter.
- Hold layout og innhold adskilt.
- La statiske ressurser være eksplisitte.
- Ikke introduser mer frontend-kompleksitet enn appen trenger.

## Når du bruker hva

- Bruk [`std.html`](../std/html.no) når du vil bygge HTML eksplisitt eller jobbe lavnivå med semantiske tagger, skjemaer, lister og tabeller.
- Bruk [`std.frontend`](../std/frontend.no) når du vil bygge gjenbrukbare komponenter, layouts og partials.
- Bruk [`std.stil`](../std/stil.no) når du vil bygge norske stilark med funksjonar som `regel(...)`, `variabel(...)`, `tema_lys(...)`, `media(...)`, `grid_kolonner(...)` og `flex_rad(...)`.
- Bruk [`std.islands`](../std/islands.no) når du vil skille mellom full side og fragment, eller modellere server/client-splitt.
- Bruk [`std.web`](../std/web.no) når du trenger request-, route- og response-hjelpere på HTTP-nivå, inkludert standard HTML-, tekst-, redirect- og tomme responsar.

## UI-kit

`std.frontend` har eit lite standard UI-kit for arbeidsflater som helpdesk, admin og dashboard.
Det gir ferdige komponentar for:

- sidehode med handlingar
- seksjonshode og panel med handlingar
- grunnkomponentar for callout, metrikk, metadata og kortlister
- aktiv navigasjonslenke med aria-current
- brødsmulesti og varsel
- skip-lenke og tydelige fokusstiler
- skjelettvisning for innlasting
- tilstandspanel med handlingar
- primær- og sekundærknappar
- verktøylinje
- ikonknappar og tooltip
- kommandohint og tastaturmerker
- avatar, brukerchip og brukerlinje
- handlingsmeny og menyelement
- tabs, segmentert kontroll og filterbar
- bulkbar for massehandlingar
- tetthetswrapper og kompakt visning
- status-badges
- tabellhandlingar for kompakte radknappar
- responsive tabellceller med mobillabels
- dashboard-statistikk
- oppsummeringsboks og nøkkelrader
- statusboard med kolonnar og sakskort
- listepanel og kompakte listerader
- fremdrift/SLA-visning
- stegindikator for saksgang
- sidebar-shell
- filterfelt
- datatabell
- feltgruppe, tekstfelt og valg
- validerte felt, feltfeil og feilsammendrag
- status- og prioritetvelger
- sorteringsvelger
- tom-tabellrad og paginering
- ticket-rad for helpdesk-lister
- split-view for liste og detalj
- detalj-layout, metadata og aktivitetslogg
- sammenleggbare disclosure-seksjonar
- kommentarboks og tomt panel
- modal
- side-skuff for hurtigvisning
- toast-region
- UI-script for modal, kopiering, live-filter og toast
- skjemafelt med required, disabled, readonly, autocomplete og feilmeldingstilstandar
- feilsammendrag, feltgrupper og standard skjemahandlinger
- HTML-lavnivå for semantiske taggar som aside, figure, figcaption, dialog, small, strong og time
- HTML islands for modal-knappar, dropdown, kopiering, confirm, autoresize, live-filter og JSON-skjema
- HTML state for statusfelt, state-panel og tom/lasting/feil-visningar
- HTML router med aktive lenker, brødsmuler og canonical metadata
- HTML templates for sidehode, artikkel, seksjon, liste-side, detaljside og dashboard

Bruk `frontend.ui_stilark()` saman med komponentane når du vil ha eit komplett, responsivt standarduttrykk utan å skrive alt stilarket sjølv.
Bruk `frontend.ui_script()` ved slutten av `<body>` når sida treng standard handlingar for knappar, filter, dialogar og små varsel.
For lågnivå markup kan komponentane blandast med `std.html`, `std.html_forms`, `std.html_islands`, `std.html_state`, `std.html_router` og `std.html_templates`.

## Praktisk konsekvens

Frontend-roadmapens første etappe er å etablere:

- prosjektstruktur
- side- og layout-kontrakt
- statiske assets
- enkel dev-flyt for frontend-ressurser
- en gradvis vei fra HTML mode til template, component, native UI og reactive mode
- reactive bindinger for tekst, verdi, checked, klasse, synlighet, events og statusfelt

Native UI er en ekte Norscode-modul i [`std/nativeui.no`](../std/nativeui.no) og brukes av `ui-render` som den kanoniske renderer-veien.
For komponent- og slot-modell er [`std/frontend.no`](../std/frontend.no) den anbefalte inngangen, [`std/reactive.no`](../std/reactive.no) er kontraktflaten for state, bindings og events, [`std/islands.no`](../std/islands.no) er kontraktflaten for server/client-splitt, og [`std/html.no`](../std/html.no) er fortsatt lavnivåflaten.
Se også [`FRONTEND_REACTIVE.md`](./FRONTEND_REACTIVE.md) for den konkrete attributtflaten.

## Videre lesing

- [`NORSK_STILARK_PLAN`](./NORSK_STILARK_PLAN.md)
- [`FRONTEND_GOLDEN_EXAMPLES`](./FRONTEND_GOLDEN_EXAMPLES.md)
- [`FRONTEND_LEARNING_PATH`](./FRONTEND_LEARNING_PATH.md)
- [`FRONTEND_FRAGMENT_MODEL`](./FRONTEND_FRAGMENT_MODEL.md)
- [`FRONTEND_MODES`](./FRONTEND_MODES.md)
