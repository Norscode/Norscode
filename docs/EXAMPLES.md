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
- [examples/web_methods.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_methods.no): HEAD, OPTIONS og metodeforhandling.
- [examples/web_auth.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_auth.no): bearer-token auth med guards.
- [examples/web_roles.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_roles.no): rolle- og rettighetsmodell med guards.
- [examples/secrets.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/secrets.no): passordhashing og secrets-håndtering.
- [examples/csrf.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/csrf.no): CSRF-tokenverifisering.
- [examples/web_cookies.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_cookies.no): secure cookies og cookie helpers.
- [examples/web_sanitize.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_sanitize.no): HTML-escaping og sikre filnavn/slugs for webbruk.
- [examples/db.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/db.no): SQLite-adapter med migreringer, query og transaksjoner.
- [examples/db_integration.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/db_integration.no): ekte databaseintegrasjon med reopen og persistert data mellom åpninger.
- [examples/db_repository.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/db_repository.no): anbefalt repository-/modelmønster med små domenefunksjoner.
- [examples/json_schema.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/json_schema.no): enkel og tydelig JSON-/schema-mapping med eksplisitte mapper-funksjoner.
- [examples/file_object_storage.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/file_object_storage.no): rå fil-lagring og strukturert objektlagring som standardmønster.
- [examples/cache.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/cache.no): liten in-memory cache-adapter for tekst, tall og bool.
- [examples/logging.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/logging.no): strukturert logging med JSON-linjer og request-id.
- [examples/metrics.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/metrics.no): counters og histogrammer for observability.
- [examples/trace.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/trace.no): trace- og span-objekter koblet til request-data.
- [examples/audit.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/audit.no): audit- og security-hendelser for sensitive operasjoner.
- [examples/observability_export.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/observability_export.no): eksportbundle for dashboards og observability-plattformer.
- [examples/web_validation.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_validation.no): query-, path- og JSON-validering med lesbare feil.
- [examples/web_dependency.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_dependency.no): dependency registration, `web.use_dependency()` og automatisk injeksjon inn i route-handlers.
- [examples/web_openapi.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi.no): OpenAPI JSON og docs-side generert fra route-signaturer.
- [examples/web_openapi_auth.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi_auth.no): OpenAPI med bearer-auth og `securitySchemes`.
- [examples/web_openapi_errors.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi_errors.no): OpenAPI med dokumenterte JSON-feilresponser.
- [examples/web_openapi_schema.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi_schema.no): OpenAPI med nestede objekt-skjemaer.
- [examples/web_api_versioning.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_api_versioning.no): API-versjonering med `/api/v1` og `/api/v2`.
- [examples/web_middleware.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_middleware.no): request/response/error middleware og startup/shutdown hooks.
- [examples/web_proxy.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_proxy.no): reverse proxy-headers og ekstern URL-normalisering.
- [examples/web_cors.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_cors.no): enkel CORS-konfigurasjon for browser-klienter.
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
