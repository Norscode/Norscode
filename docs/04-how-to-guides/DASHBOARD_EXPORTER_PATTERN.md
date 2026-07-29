# Dashboard- og exporter-mønster

Bruk dette mønsteret når du vil gjøre observability-data klare for dashbord, varslingsregler eller loggplattformer.

## Anbefalt oppdeling

- `std.metrics` for counters, gauges og histogrammer
- `std.log` for JSON-linjer til loggsystemer
- `std.trace` for request- og span-data
- `std.audit` for sikkerhetshendelser og sensitive operasjoner

## Praktisk eksport

Norscode-komponenter bør eksponere et lite, stabilt eksportformat:

- metrics som ren nøkkel/verdi-tabell
- logger som JSON-linjer
- traces som JSON-objekt eller JSON-linje per request
- audit som egen sikkerhetsstrøm

For dashbord er det ofte nok å bygge ett eksportbundle som kan deles videre til Prometheus, Grafana, Loki eller et internt observability-endepunkt.

## Eksempel

```norscode
bruk std.log som logg
bruk std.metrics som metrics
bruk std.trace som trace
bruk std.audit som audit
bruk std.web som web

funksjon start() -> heltall {
    la m = metrics.opprett()
    metrics.tell(m, "requests_total")
    metrics.histogram(m, "latency_ms", 12)

    la ctx = web.request_context("get", "/healthz", {}, {"x-request-id": "req-123"}, "")
    la spor = trace.start("GET /healthz", "trace-1", "root")
    trace.koble_request(spor, web.request_id(ctx), web.request_method(ctx), web.request_path(ctx))
    trace.event(spor, "db", "hentet status")
    trace.slutt_ok(spor)

    la logg_event = logg.info("request ferdig")
    logg.felt_tekst(logg_event, "request_id", web.request_id(ctx))
    logg.felt_tall(logg_event, "status", 200)

    la audit_event = audit.access_denied("ada", "admin", "/admin")

    la bundle: ordbok_tekst = {}
    bundle["metrics"] = json_stringify(m)
    bundle["log"] = logg.emit(logg_event)
    bundle["trace"] = json_stringify(trace.til_logg(spor))
    bundle["audit"] = audit.emit(audit_event)

    skriv(json_stringify(bundle))
    returner 0
}
```

## Når du lager dashboards

- Stabiliser feltnavn før du bygger varsler rundt dem.
- Bruk `request_id`, `service`, `route`, `status` og `latency_ms` som faste byggesteiner.
- Hold sikkerhetshendelser i en egen strøm, ikke bland dem inn i vanlige request-logger.
- Eksporter én tydelig JSON-struktur per kategori, så kan du mappe videre til ønsket plattform.
