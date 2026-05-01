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

## Regler

- bruk lokale variabler når state bare tilhører én komponent
- bruk delte moduler når flere sider trenger samme data
- la URL-en eie state som bør kunne deles
- hold cache og UI-state adskilt når det gir mening

## Hva dette betyr i praksis

- en knapp kan ha lokal loading-state
- en side kan ha delt filter-state
- en oversikt kan cache lastede data mellom navigasjoner

## Når dette er ferdig

- komponenter og sider kan håndtere enkel state uten å bli rotete
- det finnes en tydelig grense mellom lokal og delt state

Se også [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROADMAP.md).
