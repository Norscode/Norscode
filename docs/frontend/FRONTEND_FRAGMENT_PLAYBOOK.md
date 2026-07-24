# Frontend Fragment Playbook

Denne playbooken samler den korte, praktiske veien inn i server-drevne fragmenter i Norscode.

Bruk den når du vil bygge delvis rendering uten å innføre en klientruntime først.

## Når du bør bruke fragmenter

- når en route kan gi både full side og delvis svar
- når søk, filtrering eller paginering er bedre som egen server-rendret del
- når samme komponenter bør brukes i både fullside- og fragment-svar

## Standard mønster

1. Bygg innholdet én gang som en funksjon eller komponent.
2. Pakk det inn i `std.islands.fragment_shell(...)`.
3. Sjekk `std.islands.fragment_request(ctx)` hvis du vil skille mellom full side og fragment.
4. Returner enten `std.islands.fragment_response_ok(...)` eller en full `web.response_builder(...)`.

Hvis du vil slippe repetisjon i rutene, bruk gjerne `std.frontend.fragment_page(...)` som standardhjelper for dette mønsteret.

## Tre gode varianter

- `GET /sok?q=...` for serverdrevet filtrering
- `GET /poster?side=...&sort=...&kategori=...` for paginert, sortert og filtrerbar liste
- `GET /status` eller `GET /docs` for enkel fullside/fragment-dualitet
- `GET /artikler` for en samleside som lenker videre til detaljer
- `GET /ruter` for en samleside over de viktigste URL-ene
- `GET /brukere/{id}?visning=kort` for en numerisk path-basert detaljvisning
- `GET /artikler/{id}?visning=kort` for en ekstra numerisk path-basert detaljvisning

## Praktiske regler

- la URL-en fortelle hva som vises
- bruk query-parametre for filter, sortering og sidevalg
- behold samme komponenter i begge svarformer
- hold modellen JS-fri så lenge den gir verdi

## Startpunkter

- [`docs/FRONTEND_FRAGMENT_MODEL.md`](./FRONTEND_FRAGMENT_MODEL.md)
- [`docs/FRONTEND_FRAGMENT_EXAMPLES.md`](./FRONTEND_FRAGMENT_EXAMPLES.md)
- [`docs/FRONTEND_FRAGMENT_PATTERNS.md`](./FRONTEND_FRAGMENT_PATTERNS.md)
- [`examples/frontend.no`](../examples/frontend.no)

## Anbefalt leserekkefølge

1. [`docs/FRONTEND_FRAGMENT_MODEL.md`](./FRONTEND_FRAGMENT_MODEL.md)
2. [`docs/FRONTEND_FRAGMENT_PATTERNS.md`](./FRONTEND_FRAGMENT_PATTERNS.md)
3. [`docs/FRONTEND_FRAGMENT_EXAMPLES.md`](./FRONTEND_FRAGMENT_EXAMPLES.md)
4. [`examples/frontend.no`](../examples/frontend.no)
