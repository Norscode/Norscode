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

I repoet er [`std/state.no`](/Users/jansteinar/Projects/Norscode/std/state.no) den lille, anbefalte startflaten for dette mønsteret.

## Regler

- store skal ha ett tydelig ansvar
- store skal være enkel å teste
- store skal ikke blande serverdata og ren UI-state uten grunn
- oppdateringer bør være små og deterministiske

## Når store er riktig

- flere komponenter må dele samme filter
- flere sider må vise samme cachede data
- autentiserings- eller brukerstatus må leses flere steder
- du vil ha eksplisitt versjonering for å merke oppdateringer

## Når store ikke er riktig

- state tilhører bare én komponent
- data kan beregnes lokalt uten deling

## Når dette er ferdig

- delt state kan brukes uten å bli uoversiktlig
- appen har et tydelig sted for delt data
- state kan versjoneres deterministisk og testes isolert

Se også [docs/FRONTEND_STATE_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_STATE_MODEL.md) og [`std/state.no`](/Users/jansteinar/Projects/Norscode/std/state.no).
