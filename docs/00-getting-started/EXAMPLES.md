# Eksempler

Dette er de representative eksempelappene som viser de vanligste bruksmønstrene i Norscode.

## Oversikt

- [examples/basic.no](../examples/basic.no): minimal startfil, tester og standardbibliotek.
- [examples/cli.no](../examples/cli.no): CLI, filer, path, env og lagring.
- [examples/http.no](../examples/http.no): nettfri demo av `std.http`.
- [examples/web.no](../examples/web.no): path-parametre, rute-matching og dispatch.
- [examples/web_request_response.no](../examples/web_request_response.no): request_context, response_builder, header/query-hjelpere og filrespons.
- [examples/web_routes.no](../examples/web_routes.no): route-handlers med `web.route()` og `web.handle_request()`.
- [examples/web_subrouter.no](../examples/web_subrouter.no): subroutere og prefiksbaserte route-moduler.
- [examples/web_guard.no](../examples/web_guard.no): route-guards og policy-hooks.
- [examples/web_methods.no](../examples/web_methods.no): HEAD, OPTIONS og metodeforhandling.
- [examples/web_auth.no](../examples/web_auth.no): bearer-token auth med guards.
- [examples/web_roles.no](../examples/web_roles.no): rolle- og rettighetsmodell med guards.
- [examples/secrets.no](../examples/secrets.no): passordhashing og secrets-håndtering.
- [examples/csrf.no](../examples/csrf.no): CSRF-tokenverifisering.
- [examples/web_cookies.no](../examples/web_cookies.no): secure cookies og cookie helpers.
- [examples/web_sanitize.no](../examples/web_sanitize.no): HTML-escaping og sikre filnavn/slugs for webbruk.
- [examples/db.no](../examples/db.no): SQLite-adapter med migreringer, query og transaksjoner.
- [examples/db_integration.no](../examples/db_integration.no): ekte databaseintegrasjon med reopen og persistert data mellom åpninger.
- [examples/db_repository.no](../examples/db_repository.no): anbefalt repository-/modelmønster med små domenefunksjoner.
- [examples/json_schema.no](../examples/json_schema.no): enkel og tydelig JSON-/schema-mapping med eksplisitte mapper-funksjoner.
- [examples/file_object_storage.no](../examples/file_object_storage.no): rå fil-lagring og strukturert objektlagring som standardmønster.
- [examples/cache.no](../examples/cache.no): liten in-memory cache-adapter for tekst, tall og bool.
- [examples/logging.no](../examples/logging.no): strukturert logging med JSON-linjer og request-id.
- [examples/metrics.no](../examples/metrics.no): counters og histogrammer for observability.
- [examples/trace.no](../examples/trace.no): trace- og span-objekter koblet til request-data.
- [examples/audit.no](../examples/audit.no): audit- og security-hendelser for sensitive operasjoner.
- [examples/observability_export.no](../examples/observability_export.no): eksportbundle for dashboards og observability-plattformer.
- [examples/web_validation.no](../examples/web_validation.no): query-, path- og JSON-validering med lesbare feil.
- [examples/web_dependency.no](../examples/web_dependency.no): dependency registration, `web.use_dependency()` og automatisk injeksjon inn i route-handlers.
- [examples/web_openapi.no](../examples/web_openapi.no): OpenAPI JSON og docs-side generert fra route-signaturer.
- [examples/web_openapi_auth.no](../examples/web_openapi_auth.no): OpenAPI med bearer-auth og `securitySchemes`.
- [examples/web_openapi_errors.no](../examples/web_openapi_errors.no): OpenAPI med dokumenterte JSON-feilresponser.
- [examples/web_openapi_schema.no](../examples/web_openapi_schema.no): OpenAPI med nestede objekt-skjemaer.
- [examples/web_api_versioning.no](../examples/web_api_versioning.no): API-versjonering med `/api/v1` og `/api/v2`.
- [examples/web_middleware.no](../examples/web_middleware.no): request/response/error middleware og startup/shutdown hooks.
- [examples/web_proxy.no](../examples/web_proxy.no): reverse proxy-headers og ekstern URL-normalisering.
- [examples/web_cors.no](../examples/web_cors.no): enkel CORS-konfigurasjon for browser-klienter.
- Kjør samme web-stil lokalt med `norcode serve examples/web_routes.no --reload`.
- [examples/advanced.no](../examples/advanced.no): lister, slicing, sortering og løkker.
- [examples/helpdesk.no](../examples/helpdesk.no): større interaktiv app med menydrevet flyt.
- [examples/map.no](../examples/map.no): ordbøker og strukturerte data.
- [examples/struct.no](../examples/struct.no): strukturbruk og feltarbeid.

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
