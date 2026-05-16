# Cookbook

Kort, praktisk oppslagsbok for vanlige Norscode-oppgaver.

## CLI

Se hva CLI-en støtter:

```bash
./bin/nc commands
```

Kjore, sjekke, formatere og teste:

```bash
./bin/nc run app.no
./bin/nc check app.no
./bin/nc format app.no
./bin/nc test
```

## JSON

Bruk `std.ordbok` og `std.lagring` når du vil lese og skrive strukturert data:

```norscode
bruk std.lagring som lagring

funksjon start() -> heltall {
    la data: ordbok_tekst = {}
    lagring.sett_tekst(data, "navn", "Norscode")
    lagring.sett_tall(data, "versjon", 1)
    lagring.lagre("/private/tmp/demo.json", data)
    returner 0
}
```

Se også [examples/cli.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/cli.no).

## Filer

Les, skriv og sjekk filer med `std.fil`:

```norscode
bruk std.fil som fil

funksjon start() -> heltall {
    skriv(tekst_fra_bool(fil.finnes("/private/tmp/demo.txt")))
    returner 0
}
```

Se også [examples/cli.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/cli.no) og [examples/helpdesk.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/helpdesk.no).

## Skript

For små skript er `start()` nok:

```norscode
funksjon start() -> heltall {
    skriv("Kjorer et lite script")
    returner 0
}
```

Det er også helt greit å kombinere dette med `std.env`, `std.path` og `std.tekst`.

## HTTP

For API-kall, bruk `std.http`:

```norscode
bruk std.http som http

funksjon start() -> heltall {
    la respons = http.request_full_simple("GET", "https://example.com", "", 5000)
    skriv(tekst_fra_heltall(http.response_status(respons)))
    returner 0
}
```

Se [examples/http.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/http.no).

## Web

Bruk `std.web` for path-matching, dispatch og path-parametre:

```norscode
bruk std.web som web

funksjon start() -> heltall {
    la routes: ordbok_tekst = {"bruker": "GET /brukere/{id}"}
    skriv(web.dispatch(routes, "GET", "/brukere/42"))
    la params = web.path_params("/brukere/{id}/poster/{slug}", "/brukere/42/poster/hei")
    skriv(params.id)
    skriv(params.slug)
    returner 0
}
```

Se [examples/web.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web.no) og [examples/web_request_response.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_request_response.no).

For route-handlers i FastAPI-stil, se også [examples/web_routes.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_routes.no).
For subroutere og prefiksbaserte route-moduler, se [examples/web_subrouter.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_subrouter.no).
For route-guards og policy-hooks, se [examples/web_guard.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_guard.no).
For HEAD, OPTIONS og metodeforhandling, se [examples/web_methods.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_methods.no).
For bearer-token auth med guards, se [examples/web_auth.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_auth.no).
For rolle- og rettighetsmodell, se [examples/web_roles.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_roles.no).
For passordhashing og secrets-håndtering, se [examples/secrets.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/secrets.no).
For CSRF-beskyttelse, se [examples/csrf.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/csrf.no).

For secure cookies, se [examples/web_cookies.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_cookies.no).
For input-sanitizing og sikre web-strenger, se [examples/web_sanitize.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_sanitize.no).
For databasebruk med SQLite og migreringer, se [examples/db.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/db.no).
For ekte databaseintegrasjon med reopen og persistens, se [examples/db_integration.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/db_integration.no).
For repository-/modelmønstre, se [examples/db_repository.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/db_repository.no).
For enkel JSON-/schema-mapping, se [examples/json_schema.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/json_schema.no).
For fil- og objektlagring, se [examples/file_object_storage.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/file_object_storage.no).

## Cache

Bruk `std.cache` for små in-memory verdier, memoisering og per-request oppslag:

```norscode
bruk std.cache som cache

funksjon start() -> heltall {
    la c = cache.opprett()
    cache.sett(c, "bruker:42", "Ada")
    skriv(cache.hent_eller(c, "bruker:99", "ukjent"))
    returner 0
}
```

Se [examples/cache.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/cache.no).

## Logging

Bruk `std.log` for strukturert logging i JSON-linjer:

```norscode
bruk std.log som logg

funksjon start() -> heltall {
    la e = logg.info("request ferdig")
    logg.felt_tekst(e, "request_id", "req-123")
    logg.felt_tall(e, "status", 200)
    logg.emit(e)
    returner 0
}
```

Se [examples/logging.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/logging.no).

## Metrics

Bruk `std.metrics` for counters og histogrammer:

```norscode
bruk std.metrics som metrics

funksjon start() -> heltall {
    la m = metrics.opprett()
    metrics.tell(m, "requests")
    metrics.histogram(m, "latency_ms", 12)
    skriv(tekst_fra_heltall(metrics.hent(m, "requests")))
    returner 0
}
```

Se [examples/metrics.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/metrics.no).

## Tracing

Bruk `std.trace` for span- og request-sporing:

```norscode
bruk std.trace som trace

funksjon start() -> heltall {
    la spor = trace.start("handler", "trace-1", "root")
    trace.felt_tall(spor, "status", 200)
    trace.slutt_ok(spor)
    returner 0
}
```

Se [examples/trace.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/trace.no).

## Audit

Bruk `std.audit` for sikkerhetshendelser og sensitive operasjoner:

```norscode
bruk std.audit som audit

funksjon start() -> heltall {
    la e = audit.access_denied("ada", "admin", "/admin")
    audit.felt_bool(e, "blocked", sann)
    audit.emit(e)
    returner 0
}
```

Se [examples/audit.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/audit.no).

## Dashboard- og exporter-mønster

Bruk `std.metrics`, `std.log`, `std.trace` og `std.audit` sammen når du vil levere data til dashboards eller observability-plattformer:

```norscode
bruk std.log som logg
bruk std.metrics som metrics
bruk std.trace som trace
bruk std.audit som audit
bruk std.web som web

funksjon start() -> heltall {
    la m = metrics.opprett()
    metrics.tell(m, "requests_total")

    la ctx = web.request_context("get", "/healthz", {}, {"x-request-id": "req-123"}, "")
    la spor = trace.start("GET /healthz", "trace-1", "root")
    trace.koble_request(spor, web.request_id(ctx), web.request_method(ctx), web.request_path(ctx))
    trace.slutt_ok(spor)

    la e = logg.info("request ferdig")
    logg.felt_tekst(e, "request_id", web.request_id(ctx))
    logg.felt_tall(e, "status", 200)
    logg.emit(e)

    la a = audit.access_denied("ada", "admin", "/admin")
    audit.emit(a)

    skriv(json_stringify(m))
    skriv(json_stringify(trace.til_logg(spor)))
    returner 0
}
```

Se [examples/observability_export.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/observability_export.no).

For dependency injection i samme stil, se [examples/web_dependency.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_dependency.no).

For inputvalidering, se [examples/web_validation.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_validation.no).

For OpenAPI og docs, se [examples/web_openapi.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi.no).

For OpenAPI med bearer-auth, se [examples/web_openapi_auth.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi_auth.no).

For OpenAPI med dokumenterte JSON-feilresponser, se [examples/web_openapi_errors.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi_errors.no).

For OpenAPI med nestede objekt-skjemaer, se [examples/web_openapi_schema.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi_schema.no).

For API-versjonering og migrasjonsnotater, se [docs/API_VERSIONING.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/API_VERSIONING.md) og [docs/API_MIGRATION_NOTES.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/API_MIGRATION_NOTES.md).

For middleware og hooks, se [examples/web_middleware.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_middleware.no).

For lokal serverkjøring, bruk `norcode serve examples/web_routes.no --reload` eller en egen webapp-fil du vil kjøre i dev-modus.

For reverse proxy-oppsett, bruk `norcode serve ... --proxy-headers --trusted-proxy <proxy-ip>` og les forwarded headers via `web.request_header()`.

For browser-klienter og CORS, bruk `norcode serve ... --cors-origin https://app.example.com` eller la standard-CORS stå på for en enkel API-flate.

For enkel rate limiting og brute-force-beskyttelse, bruk `norcode serve ... --rate-limit-requests 120 --rate-limit-window 60`.

For containeroppsett og volum-basert kjøring, se [docs/CONTAINER.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/CONTAINER.md).
For systemd-oppsett på Linux, se [docs/SYSTEMD.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/SYSTEMD.md) og [deploy/norscode.service](/Users/jansteinar/Projects/language_handoff/projects/language/deploy/norscode.service).
For en samlet deployflyt fra release til drift, se [docs/DEPLOYMENT_PLAYBOOK.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/DEPLOYMENT_PLAYBOOK.md).
For å starte et nytt API-prosjekt, se [docs/API_SCAFFOLD.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/API_SCAFFOLD.md).

## Neste steg

- [docs/START_HER.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/START_HER.md)
- [docs/EXAMPLES.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/EXAMPLES.md)
- [docs/QUALITY.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/QUALITY.md)
