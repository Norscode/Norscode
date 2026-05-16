# Frontend Store Model

En delt app-state eller store bør være enkel og modulær.

## Mål

- dele data på tvers av komponenter og sider
- holde state forutsigbar
- unngå at alt blir globalt

## Modell

- en store er en egen modul under `state/`
- store eksponerer lesing og oppdatering eksplisitt
- komponenter abonnerer eller leser state direkte
- sideeffekter holdes utenfor selve state-dataene

## Regler

- store skal ha ett tydelig ansvar
- store skal være enkel å teste
- store skal ikke blande serverdata og ren UI-state uten grunn
- oppdateringer bør være små og deterministiske

## Når store er riktig

- flere komponenter må dele samme filter
- flere sider må vise samme cachede data
- autentiserings- eller brukerstatus må leses flere steder

## Når store ikke er riktig

- state tilhører bare én komponent
- data kan beregnes lokalt uten deling

## Når dette er ferdig

- delt state kan brukes uten å bli uoversiktlig
- appen har et tydelig sted for delt data

Se også [docs/FRONTEND_STATE_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_STATE_MODEL.md).
