# Frontend Fragment Examples

[`examples/frontend.no`](/Users/jansteinar/Projects/Norscode/examples/frontend.no) er et praktisk eksempel på server-drevne fragmenter i Norscode.

Det eksemplet viser:

- full side og fragment-svar fra samme route
- søk via query-parametre
- paginering, sortering og kategori-filter uten JS
- gjenbruk av `std.frontend` og `std.islands`
- bruk av `std.frontend.fragment_page(...)` som standardvei for fullside-/fragment-ruter

## Ruter i eksemplet

- `GET /` for en vanlig komponentdrevet startside
- `GET /docs` for fragment- og fullsideversjon av dokumentasjon
- `GET /status` for en enkel fragment-rendret statusvisning
- `GET /sok?q=...` for serverdrevet søk
- `GET /poster?side=...&sort=...&kategori=...` for paginert, sortert og filtrerbar listevisning
- `GET /artikler` for en samleside med lenker til detaljer
- `GET /ruter` for en samleside som viser viktige URL-er og deep links
- `GET /brukere/{id}?visning=kort` for en numerisk path-basert detaljvisning med fragment-flyt
- `GET /artikler/{id}?visning=kort` for en annen numerisk path-basert detaljvisning med fragment-flyt

## Hvorfor det er nyttig

- viser hvordan fullside og fragment kan bruke samme komponenter
- holder serveren som sannhet for visning og navigasjon
- gir en ren JS-fri modell for delvis rendering

Se også:

- [docs/FRONTEND_FRAGMENT_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_FRAGMENT_MODEL.md)
- [docs/FRONTEND_FRAGMENT_PATTERNS.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_FRAGMENT_PATTERNS.md)
- [docs/FRONTEND_FRAGMENT_PLAYBOOK.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_FRAGMENT_PLAYBOOK.md)
- [docs/FRONTEND_GOLDEN_EXAMPLES.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_GOLDEN_EXAMPLES.md)
- [examples/frontend.no](/Users/jansteinar/Projects/Norscode/examples/frontend.no)
