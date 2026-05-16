# Frontend Page and Module Structure

Frontendens side- og modulstruktur bør være enkel og forutsigbar.

## Mål

- gjøre det lett å finne hvor en side bor
- holde sider og støttefunksjoner ryddig delt
- unngå at alt havner i én stor fil

## Foreslått form

- `pages/` for sider
- `components/` for gjenbrukbare UI-biter
- `layouts/` for felles rammeverk
- `routes.no` for rutekobling
- `state/` for delt state og små helpers

## Regler

- én side per fil når det passer
- moduler bør ha ett klart ansvar
- route-definisjoner skal være enkle å skanne
- delte hjelpfunksjoner skal bo nær domenet de støtter

## Praktisk konsekvens

- en ny side kan legges til uten å rote til resten av appen
- navngiving og filplassering blir forutsigbar

## Når dette er ferdig

- frontend-prosjekter har en tydelig og skalerbar mappestruktur
- utviklere vet hvor sider, moduler og helper-funksjoner skal ligge

Se også:

- [docs/FRONTEND_STRUCTURE.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_STRUCTURE.md)
- [docs/FRONTEND_NAVIGATION_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_NAVIGATION_MODEL.md)
