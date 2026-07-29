# Observability-mønster

Norscode bruker nå en enkel og praktisk observability-flate for backend-appene:

- `std.web.request_id(ctx)` for request-spor
- `std.log` for strukturert logging
- `std.web.response_builder(...)` og middleware-hooks for å knytte metadata til svar

## Anbefalt mønster

Bygg loggeventer som ordbøker, legg på felter etter behov, og skriv dem som JSON-linjer.

```norscode
bruk std.log som logg

funksjon start() -> heltall {
    la e = logg.info("request ferdig")
    logg.felt_tekst(e, "request_id", "req-123")
    logg.felt_tekst(e, "route", "GET /api/brukere")
    logg.felt_tall(e, "status", 200)
    logg.emit(e)
    returner 0
}
```

I webhandlers er det naturlig å hente request-id fra `std.web` og sette den inn i loggen:

```norscode
bruk std.log som logg
bruk std.web som web

funksjon handler(ctx: ordbok_tekst) -> ordbok_tekst {
    la e = logg.info("request mottatt")
    logg.felt_tekst(e, "request_id", web.request_id(ctx))
    logg.felt_tekst(e, "path", web.request_path(ctx))
    logg.emit(e)
    returner web.response_builder(200, {"content-type": "text/plain"}, "ok")
}
```
