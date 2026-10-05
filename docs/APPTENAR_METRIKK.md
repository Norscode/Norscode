# Metrikk og spor i apptenaren (5f)

`std.metrikk` er eit metrikk-register skrive i Norscode, med Prometheus-eksponering
(`/metrics`) og OTLP/HTTP-eksport i JSON-koding. `std.apptenar` kan slå det på med éi linje.

## Slå på i apptenaren

```norscode
bruk std.apptenar som app

funksjon start() -> heltall {
    app.slå_på_metrikk()          # eller konfig {"metrikk": sann}
    returner app.lytt({"port": 8080})
}
```

Då svarar `GET /metrics` (konfig `"metrikk_sti"`) med
`Content-Type: text/plain; version=0.0.4; charset=utf-8`, og kvar førespurnad blir målt:

| Metrikk | Type | Etikettar |
|---|---|---|
| `http_requests_total` | counter | `method`, `route`, `status` |
| `http_request_duration_seconds` | histogram (standardbøtter 5 ms–10 s) | `method`, `route` |
| `process_start_time_seconds` | gauge | – |
| `process_uptime_seconds` | gauge | – |
| `apptenar_open_connections` | gauge | – |
| `apptenar_sse_streams` | gauge | – |
| `apptenar_rejected_connections_total` | counter | – |

`route` er **rutemønsteret** frå `web.route` (t.d. `/vare/{id:int}`), aldri den rå stien,
så talet på seriar er avgrensa av rutetabellen. Førespurnader utan rute (404/405,
TrustedHost-, rate- og CORS-svar) får `route="ukjend"`, statiske filer `route="statisk"`,
og ukjende metodar `method="ANNA"`. `/metrics` blir servert etter request-mellomvare og
TrustedHost, så han kan vernast med ei mellomvare som andre ruter.

## Eigne metrikkar

```norscode
bruk std.metrikk som m

funksjon registrer_ordre(kanal: tekst, sekund) -> heltall {
    # Registrering er idempotent: same namn, type og etikettar gjev same familie.
    la ordrar = m.teljar(m.standard(), "ordrar_total", "Ordrar lagde.", ["kanal"])
    la tid = m.histogram(m.standard(), "betaling_sekund", "Tid mot betalingsleverandør.",
                         ["kanal"], [0.05, 0.1, 0.5, 1.0, 5.0])
    m.tel(ordrar, [kanal])              # eller m.auk(ordrar, {"kanal": kanal}, 3)
    m.observer(tid, [kanal], sekund)
    m.sett(m.målar(m.standard(), "jobbko_lengd", "Jobbar i kø.", []), [], 12)
    returner 0
}
```

Metrikkar i `m.standard()` kjem med på `/metrics`. Eit eige register (`m.register()`) kan
skrivast ut med `m.prometheus(reg)` eller `m.prometheus_svar(reg)` (ferdig svarordbok).
Namn følgjer Prometheus (`[a-zA-Z_:][a-zA-Z0-9_:]*`, etikettar `[a-zA-Z_][a-zA-Z0-9_]*`,
`le` er reservert i histogram); etikettverdiar blir escapa (`\\`, `\"`, `\n`).

## OTLP-eksport

```norscode
bruk std.apptenar som app
bruk std.metrikk som m
bruk std.metrikk_eksport som eksport

funksjon push() -> heltall {
    eksport.eksporter_metrikk(m.standard(), {})   # ExportMetricsServiceRequest
    eksport.eksporter_spor({})                    # ExportTraceServiceRequest (bufra spans)
    returner 0
}

funksjon start() -> heltall {
    app.slå_på_metrikk()
    app.slå_på_spor()                              # éin SERVER-span per førespurnad
    returner app.lytt({"port": 8080, "periodisk": {"intervall_ms": 15000, "fn": "push"}})
}
```

- Endepunkt: `opp["endepunkt"]` (basis-URL), elles `OTEL_EXPORTER_OTLP_METRICS_ENDPOINT` /
  `OTEL_EXPORTER_OTLP_TRACES_ENDPOINT` (som dei er), elles `OTEL_EXPORTER_OTLP_ENDPOINT` +
  `/v1/metrics` eller `/v1/traces`, elles `http://localhost:4318`.
- `OTEL_SERVICE_NAME` (eller `opp["tenestenamn"]`), `OTEL_RESOURCE_ATTRIBUTES` og
  `OTEL_EXPORTER_OTLP_HEADERS` (`k=v,k=v`) blir lesne.
- Teljarar blir `sum` (monoton, kumulativ), målarar `gauge`, histogram `histogram` med
  ikkje-kumulative `bucketCounts` og `explicitBounds`. 64-bits felt (`timeUnixNano`,
  `asInt`, `count`, `bucketCounts`) er JSON-strengar, som OTLP/JSON krev.
- Spans arvar trace-id frå ein innkomande W3C `traceparent`; handlaren finn
  `ctx["__trace_id__"]` og `ctx["__traceparent__"]` for å sende han vidare. 5xx gjev
  status ERROR. Bufferen held høgst 2048 spans mellom kvar eksport.
- `m.otlp_metrikk_json(reg, opp)`, `m.otlp_spor_json(spans, opp)` og
  `m.otlp_førespurnad(signal, body, opp)` er reine (ingen nettverk) og kan testast direkte.

## Avgrensingar

- Éin tråd: registeret er ikkje trådsikkert (VM-en har éin tråd).
- Varigheit blir målt med `builtin.tid_ms()` (millisekund-oppløysing).
- `process_start_time_seconds` er tidspunktet `std.metrikk` vart teke i bruk, ikkje
  kjerna sitt starttidspunkt; minne/CPU-målarar (`process_resident_memory_bytes` o.l.)
  manglar.
- OTLP-eksporten er synkron og blokkerer lykkja medan han køyrer; berre JSON-koding
  (ikkje protobuf), og ingen gzip eller retry.
- Eldre `std.metrics` (flat `ordbok_tekst`) er uendra; nye appar bør bruke `std.metrikk`.

Test: `NORSCODE_VM_FAST=1 ./bin/nc test tests/test_std_metrikk.no`.
