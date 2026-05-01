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

For dependency injection i samme stil, se [examples/web_dependency.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_dependency.no).

For inputvalidering, se [examples/web_validation.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_validation.no).

For OpenAPI og docs, se [examples/web_openapi.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_openapi.no).

For middleware og hooks, se [examples/web_middleware.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/web_middleware.no).

For lokal serverkjøring, bruk `norcode serve examples/web_routes.no --reload` eller en egen webapp-fil du vil kjøre i dev-modus.

For reverse proxy-oppsett, bruk `norcode serve ... --proxy-headers --trusted-proxy <proxy-ip>` og les forwarded headers via `web.request_header()`.

## Neste steg

- [docs/START_HER.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/START_HER.md)
- [docs/EXAMPLES.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/EXAMPLES.md)
- [docs/QUALITY.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/QUALITY.md)
