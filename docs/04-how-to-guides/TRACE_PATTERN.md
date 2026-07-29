# Trace- og span-mønster

Bruk `std.trace` når du vil følge en request eller en arbeidsflyt gjennom flere steg.

## Start og slutt

```norscode
bruk std.trace som trace

funksjon start() -> heltall {
    la spor = trace.start("handler", "trace-1", "root")
    trace.felt_tall(spor, "status", 200)
    trace.slutt_ok(spor)
    returner 0
}
```

## Kobling mot request

`trace.koble_request(...)` tar request-id, metode og path fra `std.web`:

```norscode
bruk std.trace som trace
bruk std.web som web

funksjon handler(ctx: ordbok_tekst) -> ordbok_tekst {
    la spor = trace.start("GET /brukere/{id}", "trace-1", "root")
    trace.koble_request(spor, ctx)
    trace.event(spor, "db", "hentet bruker")
    trace.slutt_ok(spor)
    returner web.response_builder(200, {"content-type": "text/plain"}, "ok")
}
```
