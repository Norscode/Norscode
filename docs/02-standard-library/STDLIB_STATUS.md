# Standardbibliotekstatus

> Sist oppdatert: 21. juli 2026
> Omfang: `std/` og dei nært tilknytte `selfhost/std/`-modulane

Dette dokumentet er ei praktisk statusliste for standardbiblioteket i Norscode.
Målet er å skilje mellom moduler som verkar modne, moduler som er nyttige men ujamne, og moduler som treng meir standardisering før dei kan kallast stabile.

Den maskinlesbare statusmatrisa i `std/stdlib_status.no` har 61 registrerte modular: 61 stabile, 0 eksperimentelle og 0 stubbar. Statusen er dokumentert av den signerte sluttporten, ekte Windows-plattformattestasjon, Linux ARM64/QEMU, Linux/macOS native gates, Pebble DNS-01 og `test_domenehost_e2e.no`.

## Kort vurdering

`std/` er breitt og funksjonsrikt, men ikkje jamnt modna.

- Nokre kjerneområde er godt på veg mot stabilitet
- Nokre område har mykje funksjonalitet, men manglar streng test- og API-standard
- Nokre moduler er tydeleg nisje, eksperiment eller produktspesifikke
- Nokre namn ser ut til å vere parallelle variantar av same område

## Stabil eller nær stabil

Desse ser mest modne ut, og høver godt som kjerne i standardbiblioteket:

- `std/tekst.no`
- `std/liste.no`
- `std/ordbok.no`
- `std/feil.no`
- `std/path.no`
- `std/io.no`
- `std/json.no`
- `std/env.no`
- `std/dato.no`
- `std/stat.no`
- `std/streng.no`
- `std/log.no`
- `std/logging.no`
- `std/fil.no`
- `std/math.no`
- `std/base64.no`
- `std/uuid.no`
- `std/regex.no`
- `std/collections.no`
- `std/itertools.no`
- `std/heapq.no`
- `std/cache.no`
- `std/inspect.no`
- `std/test.no`
- `std/unittest.no`

## Delvis ferdig

Desse verkar nyttige og godt i bruk, men treng meir standardisering, testdekning eller tydelegare API-linje:

- `std/web.no`
- `std/http.no`
- `std/url.no`
- `std/socket.no`
- `std/ssl.no`
- `std/csrf.no`
- `std/rest.no`
- `std/graphql.no`
- `std/shutil.no`
- `std/tid.no`
- `std/timeit.no`
- `std/tempfile.no`
- `std/zip.no`
- `std/gzip.no`
- `std/tarfile.no`
- `std/csv.no`
- `std/format.no`
- `std/textwrap.no`
- `std/sesjon.no`
- `std/auth.no`
- `std/admin.no`
- `std/signal.no`
- `std/platform.no`
- `std/prosess.no`
- `std/runtime_loader.no`
- `std/runtime_gui.no`
- `std/runtime_memory.no`
- `std/runtime_security.no`
- `std/pool.no`
- `std/sched.no`
- `std/state.no`
- `std/trace.no`
- `std/warnings.no`

## Legacy, duplikat eller overlapp

Desse ser ut til å vere parallelle variantar, gamle namn eller moduler som treng konsolidering:

- `std/krypt.no` og `std/krypto.no`
- `std/dato.no` og `std/tid.no` som delvis overlappar
- `std/log.no` og `std/logging.no`
- `std/tekst.no` og `std/streng.no`
- `std/asynk.no` og `std/asynkron.no`
- `std/statistikk.no` og `std/stat.no`
- `std/fil.no` og `std/fs.no` i tillegg til `selfhost/std/fs.no`
- `std/web.no` og `selfhost/std/web.no`
- `std/path.no` og `selfhost/std/path.no`

## Produkt- og domene-spesifikke moduler

Desse er nyttige, men er ikkje “generell stdlib-kjerne” på same måte som tekst, liste og ordbok:

- `std/db.no`
- `std/orm.no`
- `std/sqlite.no`
- `std/mysql.no`
- `std/pg.no`
- `std/migrasjon.no`
- `std/frontend.no`
- `std/html.no`
- `std/html_*`-modulane
- `std/islands.no`
- `std/nativeui.no`
- `std/native/*`
- `std/compiler/*`
- `std/oauth.no`
- `std/deploy.no`
- `std/metrics.no`
- `std/validering.no`
- `std/skjema.no`
- `std/lagring.no`
- `std/graphlib.no`
- `std/graf.no`
- `std/vekta_graf.no`

## Sjølvhost-flata

Desse modulane er tett knytte til runtime og selfhost, og bør behandlast som ein eigen kategori:

- `selfhost/std/csrf.no`
- `selfhost/std/env.no`
- `selfhost/std/fs.no`
- `selfhost/std/io.no`
- `selfhost/std/path.no`
- `selfhost/std/security.no`
- `selfhost/std/web.no`

## Vedlikehald etter stabilisering

Etter 61/61 stabilt er vidare arbeid vedlikehald og utviding, ikkje ubeviste statusløft:

- modulvis eigarskap og status
- vidare testdekning per eksportert funksjon
- tydeleg canonical namn og alias-policy
- docs-generering frå kommentarar eller deklarasjonar
- rydding i overlapp og doble moduler
- klar skilje mellom generell stdlib, runtime, selfhost og produktspesifikke moduler

## Praktisk prioritet

1. Lås kjerne-modulane som stabile: `tekst`, `liste`, `ordbok`, `feil`, `path`, `io`, `json`, `env`
2. Standardiser `web`, `http`, `url`, `socket`, `ssl`
3. Rydd navneoverlapp og legacy-variantar
4. Legg til modulvis testdekning
5. Generer og publiser stdlib-docs frå kjelda

## Konklusjon

`std/` er no ei stabil 61/61-flate for Norscode 1.0. Nye modular skal framleis
gå gjennom same maskinlesbare status- og attestasjonsspor før dei får stabil
status.
