# Frontend State Model

State i Norscode-frontenden bør starte enkelt:

- komponenter kan ha lokal state
- felles state kan samles i små moduler
- all stateflyt bør være eksplisitt

## Mål

- holde komponenter enkle
- gjøre dataflyt forutsigbar
- unngå skjult mutasjon

## Modell

### Lokal state

- brukes for små, interne visninger
- egner seg for toggles, inputfelter og midlertidige UI-tilstander

### Delt state

- brukes når flere komponenter må se samme data
- bør ligge i egne moduler under `state/`

### Server data

- lastes fra backend og caches eksplisitt
- bør ikke blandes inn i UI-state uten en tydelig kontrakt

### Reaktiv kontrakt

Når komponenter trenger en tydelig markeringsflate for state, bindings og events, bruk [`std.reactive`](/Users/jansteinar/Projects/Norscode/std/reactive.no).

- `state_attr(...)` for state-markører
- `bind_text(...)`, `bind_value(...)` og `bind_checked(...)` for bindingsnavn
- `click_attr(...)` og `input_attr(...)` for event-markører
- `reactive_root(...)` og `reactive_view(...)` for tydelige reaktive containere
- `vis_hvis(...)` og `skjul_hvis(...)` for enkel betinget rendering

### Islands

Når noe skal være tydelig server-rendret, klientstyrt eller hydrert, bruk [`std.islands`](/Users/jansteinar/Projects/Norscode/std/islands.no).

- `island_root(...)` for markering av et interaktivt område
- `server_only(...)` for server-rendret innhold
- `client_only(...)` for klienteide widgets
- `hydration_root(...)` for soner som får delt state ved oppstart
- `ssr_placeholder(...)` for reserveinnhold før hydrering
- `island_shell(...)` for en eksplisitt container for små islands
- `widget_root(...)` for en tydelig reaktiv widget
- `statisk_region(...)` for en tydelig server-rendret region

## Regler

- bruk lokale variabler når state bare tilhører én komponent
- bruk delte moduler når flere sider trenger samme data
- la URL-en eie state som bør kunne deles
- hold cache og UI-state adskilt når det gir mening

## Hva dette betyr i praksis

- en knapp kan ha lokal loading-state
- en side kan ha delt filter-state
- en oversikt kan cache lastede data mellom navigasjoner
- en reaktiv komponent kan deklarere bindings og events eksplisitt i markup
- delt state kan ligge i en liten `state/`-modul med eksplisitt versjonering
- en interaktiv widget kan markeres som server-only, client-only eller hydrert
- en widget kan markeres eksplisitt som reaktiv eller statisk

## Når dette er ferdig

- komponenter og sider kan håndtere enkel state uten å bli rotete
- det finnes en tydelig grense mellom lokal og delt state
- reaktive markører og bindings kan leses uten skjult magi
- state-moduler kan oppdateres deterministisk og testes isolert
- islands kan skille server-rendering fra klientoppførsel uten skjult magi
- widgets kan skille mellom reaktiv og statisk oppførsel med tydelige markører

Se også [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_ROADMAP.md), [docs/FRONTEND_FORM_BINDING.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_FORM_BINDING.md), [`std/reactive.no`](/Users/jansteinar/Projects/Norscode/std/reactive.no) og [`std/islands.no`](/Users/jansteinar/Projects/Norscode/std/islands.no).
