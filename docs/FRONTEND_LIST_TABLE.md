# Frontend List and Table Components

Lister og tabeller er grunnleggende frontend-komponenter for data og oversikter.

## Mål

- gjøre det lett å vise samlinger av data
- holde markup konsistent
- gjøre det enkelt å gjenbruke liste- og tabellvisninger

## Listekomponenter

En listekomponent bør:

- ta inn en liste med elementer
- ha en tydelig renderer for hvert element
- støtte tom-tilstand
- kunne brukes både for navigasjon, kort og data

## Tabellkomponenter

En tabellkomponent bør:

- ta inn kolonner og rader eksplisitt
- støtte header, body og eventuelt footer
- ha en tydelig tom-tilstand
- være lett å style og responsivt tilpasse

## Retningslinjer

- bruk lister når innholdet er sekvensielt eller kort-basert
- bruk tabeller når data er kolonneorientert
- hold render-funksjoner små
- la data være eksplisitt og ikke gjemt i global state

## Når dette er ferdig

- oversiktsider kan bygges av standard komponenter
- data-visninger kan deles mellom sider
- tabellmarkup blir ikke duplisert overalt

Se også [docs/FRONTEND_COMPONENT_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_COMPONENT_MODEL.md).
