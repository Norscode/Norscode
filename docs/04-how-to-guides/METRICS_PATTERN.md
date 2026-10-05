# Metrics-mønster

Bruk `std.metrics` for counters, gauges og enkle histogrammer.

For etikettar, histogram med bøtter, Prometheus-endepunktet `/metrics` i apptenaren og
OTLP-eksport: sjå `std.metrikk` og [APPTENAR_METRIKK.md](../APPTENAR_METRIKK.md).

## Counters

```norscode
bruk std.metrics som metrics

funksjon start() -> heltall {
    la m = metrics.opprett()
    metrics.tell(m, "requests")
    metrics.øk(m, "requests", 2)
    skriv(tekst_fra_heltall(metrics.hent(m, "requests")))
    returner 0
}
```

## Histogrammer

Histogrammet holder `count`, `sum`, `min` og `max` i samme reg-tabell:

```norscode
bruk std.metrics som metrics

funksjon start() -> heltall {
    la m = metrics.opprett()
    metrics.histogram(m, "latency_ms", 12)
    metrics.histogram(m, "latency_ms", 8)
    skriv(tekst_fra_heltall(metrics.histogram_count(m, "latency_ms")))
    skriv(tekst_fra_heltall(metrics.histogram_sum(m, "latency_ms")))
    returner 0
}
```
