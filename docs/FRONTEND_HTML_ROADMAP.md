# Frontend HTML Roadmap

Dette dokumentet er avkrysningslisten for å gjøre HTML-delen av Norscode helt ferdig.
Målet er at vanlig HTML, komponenter, fragmenter og layouts skal være en stabil og naturlig vei inn i frontend uten å falle tilbake til manuell duplisering eller utydelige mønstre.

## Målbildet

- [x] HTML er den naturlige standardflaten for server-rendret UI i Norscode
- [x] `std.html`, `std.frontend`, `std.islands` og `std.web` danner en stabil og tydelig frontend-kjede
- [x] dokumentasjon, eksempler og tester forteller samme historie som koden
- [x] vanlige websider kan bygges uten å skrive rå HTML overalt

## Omgang 1: Stabiliser grunnmuren

- [x] sikre at HTML-flaten er tydelig delt mellom lavnivå, komponenter og fragments
- [x] standardisere navn og ansvar for `std.html`, `std.frontend`, `std.islands` og `std.web`
- [x] rydde bort gjenværende duplisering mellom docs, eksempler og tester
- [x] gjøre feilmeldinger og diagnose mer presise for HTML- og fragmentfeil
- [x] sikre at basisflyten er enkel å snapshot-teste

Ferdig når:

- [x] HTML-modellen er lett å forstå og vanskelig å misbruke
- [x] det finnes tydelige startpunkter for HTML, komponenter og fragmenter
- [x] feil peker til riktig lag og riktig konsept

Status: Omgang 1 er fullført.

## Omgang 2: Fullfør HTML-API-et

- [x] lås de vanligste HTML-elementene som lavnivåhelpers eller tydelige standardbyggesteiner
- [x] sikre komplett form-støtte for:
  - [x] `form`
  - [x] `input`
  - [x] `textarea`
  - [x] `select`
  - [x] `option`
  - [x] `checkbox`
  - [x] `radio`
  - [x] `switch`
  - [x] `submit`
  - [x] `reset`
- [x] gjøre lister, tabeller og navigasjon helt stabile
- [x] dokumentere når man skal bruke lavnivå `std.html` og når man skal bruke høyere nivå helpers
- [x] teste at vanlige sider og skjemaer kan bygges uten rå HTML-duplisering

Ferdig når:

- [x] de mest vanlige HTML-mønstrene kan skrives konsistent uten spesialunntak
- [x] skjemaer, tabeller og navigasjon kan bygges ryddig med standardhelpers

Status: Omgang 2 er fullført.

## Omgang 3: Fullfør komponentlaget

- [x] holde `std.frontend` som den anbefalte inngangen for gjenbrukbare komponenter
- [x] sikre at `slot(...)` og barn-komposisjon er enkel og stabil
- [x] standardisere `card`, `hero`, `alert`, `empty`, `loading`, `tabs`, `accordion`, `panel`, `sidebar`, `toolbar`, `dialog`, `toast`, `badge`, `chip` og `avatar`
- [x] sørge for at partials og komponenter kan kombineres uten uforutsigbar markup
- [x] dokumentere hvilke byggesteiner som er lavnivå og hvilke som er anbefalt nivå

Ferdig når:

- [x] en hel side kan bygges av små gjenbrukbare UI-biter uten unødvendig duplisering

Status: Omgang 3 er fullført.

## Omgang 4: Fullfør fragmentmodellen

- [x] gjøre server-drevne fragmenter til en tydelig og dokumentert standardvei
- [x] sikre full side og fragment fra samme komponenter
- [x] ha gode eksempler for:
  - [x] søk
  - [x] paginering
  - [x] sortering
  - [x] filtrering
  - [x] detaljsider
  - [x] samlesider
- [x] gjøre fragment-responser tydelige og stabile

Ferdig når:

- [x] JS-frie fragmenter er en naturlig løsning der delvis rendering faktisk gir verdi
- [x] fragmentflyten er lett å lese og lett å teste

Status: Omgang 4 er fullført.

## Omgang 5: Gjør navigasjon og routes komplett

- [x] sikre at URL-en alltid forklarer hva som vises
- [x] standardisere path-baserte og query-baserte visninger
- [x] holde samlesider og detaljsider konsistente
- [x] sørge for at direkte lenker og refresh alltid gir riktig side
- [x] dokumentere route-parametre tydelig

Ferdig når:

- [x] nye sider kan legges til uten å bryte mønsteret
- [x] navigasjon og fragmenter følger samme kontrakt

Status: Omgang 5 er fullført.

## Omgang 6: Dokumentasjon og læringsvei

- [x] holde modell, komponenter, layout, fragmenter, patterns, playbook og golden examples i sync
- [x] ha ett tydelig hovedeksempel for HTML/component/fragment-flyten
- [x] ha leserekkefølge som gjør det lett å komme i gang
- [x] fjerne gamle eller overlappende beskrivelser når de ikke lenger trengs
- [x] gjøre det enkelt for nye bidragsytere å finne riktig mønster

Ferdig når:

- [x] dokumentasjonen forteller samme historie som implementasjonen
- [x] leserekkefølgen er kort og tydelig

Status: Omgang 6 er fullført.

## Omgang 7: Kvalitet og regresjonssikring

- [x] utvide testdekning for HTML-output og fragmenter
- [x] standardisere CSS-klasser og markup-kontrakt der det gir verdi
- [x] forbedre feilmeldinger og diagnoser videre
- [x] sikre at output holder seg stabil over tid
- [x] bygge gode golden tests for representative sider

Ferdig når:

- [x] UI-laget føles som en stabil del av Norscode, ikke et sideprosjekt
- [x] små endringer gir ikke uventede output-endringer

Status: Omgang 7 er fullført.

## Omgang 8: Reduser bootstrap til minimum

- [x] fortsette å flytte frontend-relatert logikk inn i Norscode-moduler
- [x] la Python være oppstart og verktøy, ikke frontend-motor
- [x] holde `main.py` tynn og kjedelig
- [x] sikre at frontend-flaten kan brukes uten å være avhengig av bootstrap-logikk for selve rendering

Ferdig når:

- [x] HTML-veien er helt Norscode-eid i praksis
- [x] bootstrap-laget bare starter og kobler sammen, uten å eie frontend-logikken

Status: Omgang 8 er fullført.

## Anbefalt rekkefølge

1. Stabiliser grunnmuren
2. Fullfør HTML-API-et
3. Fullfør komponentlaget
4. Fullfør fragmentmodellen
5. Gjør navigasjon og routes komplett
6. Dokumentasjon og læringsvei
7. Kvalitet og regresjonssikring
8. Reduser bootstrap til minimum

## Kortversjon

HTML er ferdig når:

- [x] vanlige sider kan bygges uten rå HTML-overalt
- [x] komponenter og layouts er gjenbrukbare og stabile
- [x] fragmenter fungerer som et ekte JS-fritt mellomlag
- [x] docs, tester og implementasjon er i sync
- [x] bootstrap-laget ikke lenger eier frontend-logikken

Se også:

- [docs/FRONTEND_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_MODEL.md)
- [docs/FRONTEND_MODES.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_MODES.md)
- [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_ROADMAP.md)
- [docs/FRONTEND_NATIVE_UI_ROADMAP.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_NATIVE_UI_ROADMAP.md)
- [docs/FRONTEND_GOLDEN_EXAMPLES.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_GOLDEN_EXAMPLES.md)
