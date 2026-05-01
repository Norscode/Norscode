# Eksempler

Dette er de representative eksempelappene som viser de vanligste bruksmønstrene i Norscode.

## Oversikt

- [examples/basic.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/basic.no): minimal startfil, tester og standardbibliotek.
- [examples/cli.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/cli.no): CLI, filer, path, env og lagring.
- [examples/http.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/http.no): HTTP og JSON.
- [examples/web.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web.no): path-parametre, rute-matching og dispatch.
- [examples/web_request_response.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_request_response.no): request_context, response_builder, header/query-hjelpere og filrespons.
- [examples/web_routes.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_routes.no): route-handlers med `web.route()` og `web.handle_request()`.
- [examples/web_subrouter.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_subrouter.no): subroutere og prefiksbaserte route-moduler.
- [examples/web_guard.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_guard.no): route-guards og policy-hooks.
- [examples/web_validation.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_validation.no): query-, path- og JSON-validering med lesbare feil.
- [examples/web_dependency.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_dependency.no): dependency registration, `web.use_dependency()` og automatisk injeksjon inn i route-handlers.
- [examples/web_openapi.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi.no): OpenAPI JSON og docs-side generert fra route-signaturer.
- [examples/web_middleware.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_middleware.no): request/response/error middleware og startup/shutdown hooks.
- [examples/web_proxy.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_proxy.no): reverse proxy-headers og ekstern URL-normalisering.
- Kjør samme web-stil lokalt med `norcode serve examples/web_routes.no --reload`.
- [examples/advanced.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/advanced.no): lister, slicing, sortering og løkker.
- [examples/helpdesk.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/helpdesk.no): større interaktiv app med menydrevet flyt.
- [examples/map.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/map.no): ordbøker og strukturerte data.
- [examples/struct.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/struct.no): strukturbruk og feltarbeid.

## Anbefalt leserekkefolge

1. `basic.no`
2. `cli.no`
3. `http.no`
4. `advanced.no`
5. `helpdesk.no`

## Hva de dekker

- språkets minste end-to-end-flyt
- variabler, funksjoner og tester
- filsystem og config
- JSON og nettverk
- lister, slicing og sortering
- større appstruktur
