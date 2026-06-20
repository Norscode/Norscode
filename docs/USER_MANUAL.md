# Norscode Brukermanual

Dette er den praktiske manualen for å bruke Norscode lokalt. Målet er at du skal kunne installere, kontrollere, køyre, teste, bygge og feilsøkje utan å gå inn i gamle Python- eller C-løyper.

## Kort Fortalt

Norscode er eit norsk programmeringsspråk og verktøysett med native CLI og selfhost-runtime.

Normal kjede:

```text
.no -> lexer/parser/semantic/bytecode -> NCB JSON -> selfhost/vm.no
```

Normal verktøyflate:

- `./bin/nc`
- `dist/norscode_native`
- `selfhost/vm.no`
- `selfhost/json.no`
- `std/`
- `NorsDB/`

Historisk C og gamle migreringsspor skal ikkje brukast som normal arbeidsveg.

## Installasjon Og Kontroll

Kort installasjonsguide ligg i [`INSTALL.md`](../INSTALL.md).

Køyr frå repo-rota:

```bash
./bin/nc --help
./bin/nc doctor
./bin/nc verify-seed
```

For lokal installasjon er normal brukarsti:

```bash
nc --help
nc doctor
nc verify-seed
```

Når installasjonen er frisk, skal `nc --help` vise at CLI-en er C/Python-fri, og `nc doctor` skal gi systemsjekk utan å krevje C-kompilator eller Python-pipeline.

## CLI-Kommandoar

Dei viktigaste kommandoane:

```bash
./bin/nc run app.no
./bin/nc check app.no
./bin/nc compile app.no out.ncb.json
./bin/nc build app.no out.ncb.json
./bin/nc format app.no --check
./bin/nc lint app.no --check
./bin/nc test
./bin/nc feature-check app.no
```

Vedlikehald:

```bash
./bin/nc maintenance status
./bin/nc maintenance verify
./bin/nc maintenance report
./bin/nc maintenance report-json
```

Selfhost og verifikasjon:

```bash
./bin/nc selfhost-bootstrap-gate
./bin/nc bootstrap-self
./bin/nc verify-selvstendighet
./bin/nc verify-seed
./bin/nc ci
```

Server og prosjekt:

```bash
./bin/nc serve app.no --port 8080
nc startproject mittprosjekt
nc startapp minapp
```

## Køyre, Sjekke Og Teste Kode

Køyr eit program:

```bash
./bin/nc run app.no
```

Sjekk syntaks og semantikk:

```bash
./bin/nc check app.no
```

Køyr testane:

```bash
./bin/nc test
```

Sjekk nye funksjonar utan C/Python:

```bash
./bin/nc feature-check app.no
```

`feature-check` er den viktigaste kommandoen når du legg til ny Norscode-funksjonalitet. Han sjekkar aktiv flate og køyrer relevante kontroller utan å bruke gamle C- eller Python-steg.

## Første Program

Lag ei fil, til dømes `hei.no`:

```norscode
funksjon start() -> heiltall {
    skriv("Hei frå Norscode")
    returner 0
}
```

Køyr:

```bash
./bin/nc run hei.no
```

Nokre eldre filer bruker `heltall`. Ny dokumentasjon bruker `heiltall` der det er naturleg, men runtime toler framleis fleire etablerte variantar i eksisterande kode.

## Språkgrunnlag

Vanlege byggesteinar:

```norscode
la namn = "Norscode"
la alder: heiltall = 2
la aktiv = sann

hvis aktiv {
    skriv(namn)
} ellers {
    skriv("ikkje aktiv")
}
```

Løkker:

```norscode
la i: heiltall = 0
mens i < 3 {
    skriv("runde")
    i = i + 1
}
```

Funksjonar:

```norscode
funksjon helsing(namn: tekst) -> tekst {
    returner "Hei " + namn
}
```

## Modular

Importer ein modul med `bruk` når modulen er del av standardbiblioteket:

```norscode
bruk std.json som json

funksjon start() -> heiltall {
    la data = json.parse("{\"namn\":\"Norscode\"}")
    skriv(data["namn"])
    returner 0
}
```

Nokre prosjektfiler bruker `importer` for lokale modular:

```norscode
importer norsdb_core
importer norsdb_schema
```

Bruk mønsteret som finst i mappa du arbeider i. Standardbibliotek bruker normalt `bruk std.modul som alias`.

## Pakker Og Standardbibliotek

Viktige standardmodular:

- `std.json`: JSON parse/stringify.
- `std.web`: enkel web/server-hjelp.
- `std.asynk`: worker-konfig, oppgåvekø, bakgrunnsjobbar, timeout/retry og streaming responses.
- `std.multiprocessing`: deterministisk prosess-/pool-/queue-kontrakt utan Python/C i aktiv flate.
- `std.stdlib_status`: maskinlesbar status for kva som er `stabil`, `eksperimentell` eller `stub`.
- `std.db`: databaseflate brukt av NorsDB og smoke-testar.
- `std.tekst`: tekstoperasjonar.
- Python-liknande modular som `std.sys`, `std.errno`, `std.multiprocessing`, `std.wsgiref`, `std.socketserver` og `std.plistlib` skal halde aktiv flate fri for `python3` og testast av `tests/test_python_independence_std.no`.
- `std_liste`, `std_ordbok`, `std_math`, `std_io`: installerte runtime-pakker i lokal distribusjon.

Eksempel med JSON:

```norscode
bruk std.json som json

funksjon start() -> heiltall {
    la payload = "{\"navn\":\"Norscode\",\"aktiv\":true}"
    la parsed = json.parse(payload)
    skriv(parsed["navn"])
    returner 0
}
```

Pakkesystemet (`norspkg`) skal kjennast naturleg i same kategori som `pip`: manifest, resolver, lockfile, install, publish, cache, søk/info og tydelege feil. Maturity-gaten er:

```bash
./bin/nc run tests/norspkg_tests.no
```

For standardbibliotek-status:

```norscode
bruk std.stdlib_status som ss

funksjon start() -> heiltall {
    la s = ss.samandrag()
    skriv("Stabile modular: " + tekst(s.stabil))
    returner 0
}
```

For streaming-respons:

```norscode
bruk std.asynk som asynk

funksjon start() -> heiltall {
    la stream = asynk.stream_lag("text/event-stream")
    asynk.stream_skriv(stream, "data: klar\n\n")
    asynk.stream_avslutt(stream)
    la respons = asynk.streaming_response(stream)
    skriv(respons["__headers__"]["transfer-encoding"])
    returner 0
}
```

`./bin/nc serve` kan serve statiske filer med produksjonsheaderar når `NC_SERVE_STATIC_DIR` peikar til ei mappe og URL-en startar med `/static/`:

```bash
NC_SERVE_STATIC_DIR=frontend/assets/icons ./bin/nc serve examples/web_cors.no --port 8080
```

Upload, sessions og deploy har eigne produksjonskontraktar:

- `std.opplasting.produksjonsstatus()` dekker multipart, JSON-upload, raw body, storleiksgrense, typevalidering, trygt filnamn og lagring.
- `std.sesjon.sikkerheitsstatus()` dekker serverside session, `HttpOnly`, `SameSite=Lax`, valfri `Secure`, TTL, flash og opprydding.
- `std.deploy.produksjonsmønster(cfg)` samlar workers, helse, statics, env-fil, rollback og graceful shutdown.
- `std.multiprocessing.produksjonsstatus()` samlar prosess, pool, queue, pipe, event, lock og value som aktiv runtime-kontrakt.

LSP-serveren i [selfhost/lsp/server.no](/Users/jansteinar/Projects/Norscode1/selfhost/lsp/server.no) annonserer no hover, completion, definition, document symbols og document formatting i same aktive Norscode-flate.

## Prosjekt Og Apper

Opprett prosjekt:

```bash
nc startproject mittprosjekt
```

Opprett app-modul:

```bash
nc startapp minapp
```

Etter generering:

```bash
./bin/nc check app.no
./bin/nc serve app.no --port 8080
./bin/nc feature-check app.no
```

## Server

Start server:

```bash
./bin/nc serve app.no --port 8080
```

Typisk serverfil bruker `std.web`:

```norscode
bruk std.web som web
```

Serverlaget har same standardflyt som moderne Python-rammeverk:

- `GET`, `HEAD` og `OPTIONS` blir handtert gjennom same rute-kontrakt.
- JSON- og tekstsvar får trygg `content-type` når appen ikkje set han sjølv.
- Feilsvar frå serverruta er JSON (`{"error": ...}`) og passar API-klientar.
- Produksjonsheaders blir lagt på av servermotoren: `x-content-type-options`, `referrer-policy`, `x-frame-options`, `cache-control` og `server`.
- `x-request-id` blir ført vidare når klienten sender han, elles blir han generert.
- CORS/preflight svarer med `allow`, `access-control-allow-methods` og `access-control-allow-headers`.

Når serveren ikkje startar:

- køyr `./bin/nc check app.no`
- sjekk at porten ikkje er opptatt
- køyr `./bin/nc doctor`
- sjekk at fila ikkje bruker gamle Python/C-baserte verktøy

## NorsDB

NorsDB ligg i `NorsDB/` og skal kunne røyk-testast frå normal runtime.

Køyr:

```bash
./bin/nc run NorsDB/norsdb_smoke.nors
```

For databasearbeid skal ny kode bruke støtta Norscode- og standardbibliotekflater. Unngå å innføre Python-konvertering, C-VM-steg eller nye `.py`-verktøy i normal løype.

## AI

Norscode har ei aktiv `std.ai`-flate for lokal, deterministisk AI-kontrakt. Ho krev ikkje nettverk, Python, C eller provider-nøklar i normal testløype.

Køyr:

```bash
./bin/nc run examples/ai.no
./bin/nc run tests/test_ai.no
```

Støtta kontraktar:

- chat-respons med `ai.chat(...)` og `ai.chat_meldinger(...)`
- embeddings med `ai.embedding(...)`
- moderering med `ai.moderer(...)`
- verktøykall med `ai.verktøy(...)` og `ai.verktøy_kall(...)`
- enkel agent-plan med `ai.agent_plan(...)`

Provider-backends kan koblast på seinare bak same kontrakt. Standardflata er med vilje lokal og deterministisk slik at CI og release framleis er sjølvstendig.

## Bygg Og Bytecode

Kompiler til NCB JSON:

```bash
./bin/nc compile app.no app.ncb.json
```

Alias:

```bash
./bin/nc build app.no app.ncb.json
```

Den normale selfhost-vegen er at NCB JSON køyrer via `selfhost/vm.no`.

## Native Bygg

Native bygg er del av Norscode-verktøyflata:

```bash
./bin/nc bygg-native app.no out
./bin/nc verify-omgang6
./bin/nc verify-omgang6b
```

For vanleg utvikling er `run`, `check`, `test` og `feature-check` nok. Bruk native bygg når du arbeider med runtime, release eller selfhost-paritet.

## Vedlikehald

Vanlege sjekkar:

```bash
./bin/nc maintenance status
./bin/nc maintenance verify
./bin/nc maintenance report-json
```

`maintenance report-json` er nyttig for maskinlesbar status. Feltet `stage0_seed_ok` viser om stage-0 seed er frisk.

## Feilsøking

Start alltid med:

```bash
./bin/nc doctor
./bin/nc check app.no
./bin/nc feature-check app.no
```

Vanlege symptom:

- **Filen køyrer ikkje**: sjekk syntaks med `check`.
- **Import feiler**: sjekk modulnamn, alias og om modulen finst i `std/`, `selfhost/` eller prosjektmappa.
- **Server startar ikkje**: sjekk port, `serve`-kommando og `std.web`-bruk.
- **Test feiler lokalt**: køyr `./bin/nc test` og deretter relevant enkeltfil med `run`.
- **Selfhost-status er uklar**: køyr `./bin/nc maintenance verify`.

## Kva Du Ikkje Skal Bruke Som Normalveg

I normal utvikling skal du ikkje innføre:

- nye Python-verktøy for kompilering eller køyring
- nye C-VM-steg i CI
- `ncb_to_c` i produksjonsflyt
- `.py`, `.c` eller `.h` i aktiv `tools/`, `selfhost/`, `bin/`, `bootstrap/` eller CI-flate

Historiske spor høyrer heime i `archive/`.

## Rask Sjekkliste

Før du sender endringar:

```bash
./bin/nc check app.no
./bin/nc feature-check app.no
./bin/nc test
```

For docs-only endringar:

```bash
./bin/nc maintenance docs
```
