# Norscode 10/10 Modenhetsplan

Dette dokumentet gjer 10/10 til ein konkret kontrakt, ikkje berre ein ambisjon.

Målet er at kvart område berre kan kallast 10/10 når det har:

- tydeleg brukarflate
- dokumentert kontrakt
- automatisert verifisering
- minst eitt praktisk eksempel
- release- eller CI-gate som fangar regresjon
- ingen aktiv avhengigheit til Python, C, NCBB-konvertering eller nye C-VM-steg

Normal kjede er framleis:

```text
.no -> lexer/parser/semantic/bytecode -> NCB JSON -> selfhost/vm.no
```

## Poengregel

Eit område kan ikkje løftast i status før desse tre bevisa finst:

- **Kodebevis:** funksjonen finst i aktiv Norscode-flate.
- **Bruksbevis:** dokumentasjon eller eksempel viser normal bruk.
- **Gatebevis:** `./bin/nc test`, `./bin/nc feature-check [fil.no ...]`, `./bin/nc maintenance verify`, relevant selfhost-gate eller CI fangar regresjon.

Samla modenheitsbevis kan sjekkast med:

```bash
./bin/nc maintenance maturity
```

10/10 betyr ikkje at alt er ferdig for alltid. Det betyr at området er trygt nok til at ein vanleg brukar kan installere, forstå, bruke, feilsøke og oppgradere det utan intern prosjektkunnskap.

## Status Og Mål

| Område | No | 10/10-kriterium | Primær gate |
| --- | ---: | --- | --- |
| Språkkjerne (parser, AST, semantikk) | 10/10 | Stabil syntakskontrakt, semantiske feilmeldingar, regresjonstestar og dokumenterte breaking-change-reglar | `./bin/nc test` + `./bin/nc feature-check` |
| CLI og verktøy | 10/10 | Alle daglege kommandoar er dokumenterte, har selftest, og gir handlingsretta feil | `./bin/nc selftest` + `./bin/nc doctor` + `tests/test_cli_maturity.no` |
| Dokumentasjon | 10/10 | Ein hovudsti for brukar, utviklar, release og feilsøking; arkiv er tydeleg merka | dokumentasjonsindeks + release-sjekkliste |
| Installasjon og distribusjon | 10/10 | Reproduserbar release for macOS, Linux og Windows med checksums, rollback og installasjonstest | release-workflows + `./bin/nc doctor` + `tests/test_distribution_maturity.no` |
| Standardbibliotek | 10/10 | Stabil `std`-API for tekst, fil, JSON, HTTP, tid, logging, test, db, AI, statusmatrise, async/runtime og sikker basis | `./bin/nc test` + `tests/test_stdlib_maturity.no` + `tests/test_stdlib_status_matrix.no` |
| Web/API-rammeverk | 10/10 | Routing, request/response, validering, auth, middleware, statiske filer, streaming, OpenAPI og server-eksempel er dekte | web smoke + scaffold-test + `tests/test_serve_runner_production_defaults.no` |
| Database (NorsDB) | 10/10 | CRUD, skjema, transaksjon, indeks, snapshot, recovery og sikkerheitsmodell har smoke/regresjon | `./bin/nc run NorsDB/norsdb_smoke.nors` |
| IDE/LSP-støtte | 10/10 | Initialize, diagnostics, hover, completion, symbols og formattering fungerer i VS Code | `./bin/nc lsp` + `tests/test_lsp_maturity.no` |
| Økosystem/pakker | 10/10 | `norspkg` har manifest, resolver, lockfile, cache, søk, info, publish/install, feilkontrakt og dokumentert registry-protokoll | `tests/norspkg_tests.no` |
| Produksjonsbruk | 10/10 | Observability, config, secrets, sikker standard, deploy-guide, rollback og produksjonseksempel | `./bin/nc maintenance verify` + `tests/test_production_maturity.no` |

## 10/10 Backlog

### Språkkjerne

- Lås aktiv syntakskontrakt for parser og AST.
- Legg regresjonstest for kvar tidlegare kritisk parser-/semantic-feil.
- Dokumenter feilkodar og eksempel på gode semantiske feilmeldingar.

### CLI Og Verktøy

- Gjer `doctor`, `lsp`, `format`, `lint`, `test`, `feature-check` og `maintenance verify` til same kvalitetsnivå: klar output, exit-kodar og feilsøkingsråd.
- Legg kort “kva gjer eg no?”-tekst ved vanlege feil.
- Sørg for at `./bin/nc commands` alltid speglar faktisk kommando-flate.

### Dokumentasjon

- Hald `docs/INDEX.md` som menneskeleg startpunkt.
- Hald `docs/DOCUMENTATION_INDEX.md` som vedlikehaldsindeks.
- Flytt eller merk historiske statuspåstandar som arkiv når dei ikkje lenger er gjeldande.

### Installasjon Og Distribusjon

- Gjer release-sjekklista obligatorisk før tag.
- Verifiser installert release med `nc --version`, `nc doctor` og eit lite `run/check/test`-sett.
- Dokumenter rollback med konkrete kommandoar per plattform.

### Standardbibliotek

- Definer stabil API-kontrakt for kjernepakker.
- Legg smoke-eksempel per modul.
- Lag statusmatrise som viser `klar`, `eksperimentell` eller `arkiv`.
- `std.stdlib_status` er maskinlesbar statusmatrise med `stabil`, `eksperimentell` og `stub`.
- `std.ai` gir chat-, embedding-, modererings-, tool- og agent-kontrakt utan Python/C eller nettverkskrav.
- `std.asynk` gir worker-konfig, oppgåvekø, bakgrunnsjobbar, retry/timeout og streaming-respons som aktiv Norscode-kontrakt.
- `std.multiprocessing` skal ha eksplisitt produksjonsstatus for prosess, pool, queue, pipe, event, lock og value.
- Python-liknande std-modular skal vere Norscode-sjølvstendige i aktiv flate og låsast med `tests/test_python_independence_std.no`.

### Web/API-Rammeverk

- Fullfør FastAPI-liknande standardflyt: routing, dependency, middleware, auth, validering og OpenAPI.
- Legg ende-til-ende eksempel for eit lite API med auth og database.
- Knyt scaffold-output til testbar kontrakt.
- Direkte web-smoke skal vere grøn for `tests/test_web_*.no`.
- `serve_runner` skal kunne serve `/static/...` frå `NC_SERVE_STATIC_DIR` med MIME, cache-header og sikkerheitsheaderar.
- Upload, session og deploy skal ha eksplisitte produksjonskontraktar i `std.opplasting`, `std.sesjon` og `std.deploy`.

### Database (NorsDB)

- Løft NorsDB frå MVP til verifisert komponent med recovery- og transaksjonstestar.
- Dokumenter datamodell, filformat, feilkodar og sikkerheitsgrenser.
- Legg integrasjonseksempel mot web/API.

### IDE/LSP-Støtte

- Dokumenter støtta LSP-metodar.
- Test JSON-RPC initialize, diagnostics, hover, completion og symbols.
- Formattering og document symbols skal vere del av aktiv capability-flate i `selfhost/lsp/server.no`.
- Knytt VS Code-utvidinga til same selftest-kontrakt.

### Økosystem/Pakker

- Lås manifestformat og lockfile.
- Gjer install/update/publish-flyten repeterbar.
- Dokumenter registry-protokoll og offline-cache.
- `norspkg` sin selftest skal dekke søk, info, publish, duplikatfeil, lockfile, install og cache.
- `norspkg` skal publisere kommando-/feilkontrakt for daglege kommandoar, publish/yank, offline-cache og maskinlesbart samandrag.

### Produksjonsbruk

- Lag produksjonsguide for konfig, secrets, logging, metrics, deploy og rollback.
- Legg eit komplett eksempel som kan køyrast lokalt utan Python/C.
- Gjer sikker standard tydeleg: ingen debug-defaults, klare feil og trygge headers/session-val.

## Første Milepælar

### Milepæl A: 8/10-Golv Over Heile Flata

- Språkkjerne, CLI og dokumentasjon held minst dagens nivå med gates.
- Installasjon, stdlib, web, NorsDB, LSP, pakker og produksjon har kvar sin smoke-test eller selftest.
- `./bin/nc maintenance maturity` finn alle 10/10-bevisfilene.
- `tests/test_maturity_10_10_gates.no` går grønt i `./bin/nc test`.
- Direkte web/API-smoke går grønt for `tests/test_web_*.no`.
- `./bin/nc maintenance verify` viser alle kritiske statuspunkt.

### Milepæl B: 9/10-Releasekandidat

- Alle område har dokumentert kontrakt og eksempel.
- Release-sjekklista kan køyrast mekanisk.
- Minst éin ende-til-ende app demonstrerer CLI, stdlib, web, NorsDB og logging.
- Alle område i status-tabellen er minst 9/10 og har aktiv gate i repoet.

### Milepæl C: 10/10-Produktflate

- Brukar kan installere, lage prosjekt, legge til app, køyre test, bygge release og rulle tilbake utan intern hjelp.
- CI fangar regresjon i språk, CLI, stdlib, web, NorsDB, LSP og pakker.
- Aktiv flate er sjølvstendig utan Python og C.

## Arbeidsregel

Når vi legg til ein funksjon for å løfte modenheit:

1. Implementer i aktiv Norscode-flate.
2. Legg eksempel eller dokumentasjon same dag.
3. Køyr `./bin/nc feature-check [fil.no ...]` for relevante filer.
4. Køyr relevant smoke/selftest.
5. Oppdater denne planen berre når gatebeviset finst.
