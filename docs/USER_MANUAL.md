# Norscode Brukarmanual

Dette er den praktiske manualen for å bruke Norscode lokalt. Målet er at du skal kunne installere, kontrollere, køyre, teste, byggje og feilsøkje utan å gå inn i gamle Python- eller C-løyper.

## Kort fortalt

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

## Installasjon og kontroll

Kort installasjonsguide ligg i [installasjonsguiden](../INSTALL.md).

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

## CLI-kommandoar

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
./bin/nc repl '1 + 2'
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green
./bin/nc local-green --strict
./bin/nc local-green --strict --list
./bin/nc local-green --strict -l
./bin/nc local-green --list
./bin/nc local-green -l
./bin/nc local-green --help
./bin/nc local-green -h
./bin/nc stage0-release-assets --platform macos-arm64
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
./bin/nc selvstendighet
./bin/nc verify-seed
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green
./bin/nc local-green --strict
./bin/nc local-green --strict --list
./bin/nc local-green --strict -l
./bin/nc local-green --list
./bin/nc local-green -l
./bin/nc local-green --help
./bin/nc local-green -h
./bin/nc stage0-release-assets --platform macos-arm64
./bin/nc ci
```

Server og prosjekt:

```bash
./bin/nc serve app.no --port 8080
nc startproject mittprosjekt
nc startapp minapp
```

## Køyre, sjekke og teste kode

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

`./bin/nc run <fil.no>` køyrer også vanlege `bruk`/`importer`-program med bundla imports. Selfhost-kjerneimportar held fram på normal runtime-veg.

Evaluer eit lite uttrykk direkte, eller start interaktiv REPL utan argument:

```bash
./bin/nc repl '1 + 2'
./bin/nc repl
```

Sjekk lokal release-/GitHub-klargjering utan å publisere:

```bash
./bin/nc release-preflight
./bin/nc release-preflight --strict
```

Køyr samla lokal grønnliste utan å publisere:

Ho køyrer release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate.

```bash
./bin/nc local-green
./bin/nc local-green --strict
./bin/nc local-green --strict --list
./bin/nc local-green --strict -l
./bin/nc local-green --list
./bin/nc local-green -l
./bin/nc local-green --help
./bin/nc local-green -h
```

`--list` og `-l` viser grønnliste-stega med kommandoar utan å køyre dei.

Bygg stage-0 release-artefaktar utan å skrive i repo-rota:

```bash
./bin/nc stage0-release-assets --platform macos-arm64
./bin/nc stage0-release-assets --platform linux-x86_64 --out-dir release-artifacts/stage0
```

Kommandoen skriv ELF og flyttbar `.sha256` til `release-artifacts/stage0/` eller vald `--out-dir`.

## Første program

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

Nokre eldre filer brukar `heltall`. Ny dokumentasjon brukar `heiltall` der det er naturleg, men runtime toler framleis fleire etablerte variantar i eksisterande kode.

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

Nokre prosjektfiler brukar `importer` for lokale modular:

```norscode
importer norsdb_core
importer norsdb_schema
```

Bruk mønsteret som finst i mappa du arbeider i. Standardbibliotek brukar normalt `bruk std.modul som alias`.

## Pakkar og standardbibliotek

Viktige standardmodular:

- `std.json`: JSON parse/stringify.
- `std.web`: web/server-hjelp for request, routing, standard responsar, cookies og produksjonsheaders.
- `std.auth`: innlogging, session-token, roller og ferdige 401/403/login/logout-responsar med session-cookie.
- `std.security`: samla sikkerheitsgate for auth, session, CSRF, headers, cookies, secrets, random, audit, admin-panel og deploy-health.
- `std.web_app_stack`: samla webflyt for innstillingar, migrasjon, auth/session, admin og standard JSON-responsar.
- `std.statisk`: statiske filer med MIME, cache, ETag og trygg "ikkje statisk"-flyt vidare til app-ruter.
- `std.deploy`: produksjonsoppsett for systemd, supervisor, nginx, helsesjekk og rollback.
- `std.rest`: REST-hjelpere for JSON-svar, CRUD-viewset, paginering, feilsvar og API-responsar som `svar`, `ok_objekt`, `ok_liste`, `akseptert` og `oppretta_med_location`.
- `std.admin`: CRUD-admin for registrerte modellar, med escaped HTML-output, no-store admin-headers, oversiktsstatistikk, tom-tilstand og CSRF-sikre admin-POST-hjelparar.
- `std.mw`: middleware for CORS, rate limiting, tryggleiksheaders, logging, komprimering og cache, med pipeline-klare konfigurasjonar.
- `std.asynk`: worker-konfig, oppgåvekø, bakgrunnsjobbar, timeout/retry og streaming responses.
- `std.multiprocessing`: deterministisk prosess-/pool-/queue-kontrakt utan Python/C i aktiv flate.
- `std.stdlib_status`: maskinlesbar status for kva som er `stabil`, `eksperimentell` eller `stub`.
- `std.db`: databaseflate brukt av NorsDB og smoke-testar.
- `std.tekst`: tekstoperasjonar.
- Python-liknande modular som `std.sys`, `std.errno`, `std.multiprocessing`, `std.wsgiref`, `std.socketserver` og `std.plistlib` skal halde aktiv flate fri for `python3` og testast av `tests/test_python_independence_std.no`.
- `std_liste`, `std_ordbok`, `std_math`, `std_io`: installerte runtime-pakkar i lokal distribusjon.

Døme med JSON:

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

- `std.opplasting.produksjonsstatus()` dekker multipart, JSON-upload, raw body, storleiksgrense, typevalidering, trygt filnamn, standard JSON-responsar og lagring.
- `std.sesjon.sikkerheitsstatus()` dekker serverside session, `HttpOnly`, `SameSite=Lax`, valfri `Secure`, TTL, flash, opprydding og cookie-headerar på standard `std.web`-responsar.
- `std.deploy.produksjonsmønster(cfg)` samlar workers, helse, statics, env-fil, rollback og graceful shutdown.
- `std.multiprocessing.produksjonsstatus()` samlar prosess, pool, queue, pipe, event, lock og value som aktiv runtime-kontrakt.

LSP-serveren i [selfhost/lsp/server.no](../selfhost/lsp/server.no) annonserer no hover, completion, definition, document symbols og document formatting i same aktive Norscode-flate.

## Prosjekt og appar

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

Typisk serverfil brukar `std.web`:

```norscode
bruk std.web som web
```

Serverlaget har same standardflyt som moderne web-rammeverk:

- `GET`, `HEAD` og `OPTIONS` blir handtert gjennom same rute-kontrakt.
- JSON- og tekstsvar får trygg `content-type` når appen ikkje set han sjølv.
- Vanlege svar kan lagast direkte med `response_html`, `response_text_plain`, `response_redirect_found`, `response_redirect_see_other` og `response_no_content`.
- Ekstra responsheaderar kan leggjast på med `response_with_header`.
- Feilsvar frå serverruta er JSON (`{"error": ...}`) og passar API-klientar.
- Produksjonsheaders blir lagt på av servermotoren: `x-content-type-options`, `referrer-policy`, `x-frame-options`, `cache-control` og `server`.
- `x-request-id` blir ført vidare når klienten sender han, elles blir han generert.
- CORS/preflight svarer med `allow`, `access-control-allow-methods` og `access-control-allow-headers`.

Når serveren ikkje startar:

- køyr `./bin/nc check app.no`
- sjekk at porten ikkje er oppteken
- køyr `./bin/nc doctor`
- sjekk at fila ikkje brukar gamle Python/C-baserte verktøy

## Nettsidekomponentar

`std.html` og `std.frontend` kan byggje server-rendert HTML frå Norscode. For vanlege nettsidehandlingar kan sida bruke Norscode-genererte `data-nc-*`-attributt og `html.script_standard()`:

```norscode
bruk std.html som html

funksjon side() -> tekst {
    returner html.div(
        "",
        html.toggle_knapp("Vis", "panel", "is-open")
            + html.div(html.id_attr("panel"), html.p("", html.escape("Innhald")))
            + html.script_standard()
    )
}
```

Standardhjelparane dekkjer knappar og små interaksjonar som toggle, faner, autosubmit, JSON-skjema, stadfesting, deaktivering av submit, modal/dialog, nedtrekksmeny, kopiering, auto-resize av tekstfelt og livefilter.

JSON-skjema kan også få status- og resultatfelt:

```norscode
html.form(
    html.attr("method", "post")
        + html.json_form_attrs("/api/tickets", "resultat")
        + html.form_status_attrs("status", "Sender", "Lagret", "Feil"),
    html.input(html.attr("name", "tittel"))
        + html.submit("", "Send")
)
```

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

Provider-backends kan koplast på seinare bak same kontrakt. Standardflata er med vilje lokal og deterministisk slik at CI og release framleis er sjølvstendig.

## Bygg og bytecode

Kompiler til NCB JSON:

```bash
./bin/nc compile app.no app.ncb.json
```

Alias:

```bash
./bin/nc build app.no app.ncb.json
```

Den normale selfhost-vegen er at NCB JSON køyrer via `selfhost/vm.no`.

## Native bygg

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
- **Import feilar**: sjekk modulnamn, alias og om modulen finst i `std/`, `selfhost/` eller prosjektmappa.
- **Server startar ikkje**: sjekk port, `serve`-kommando og `std.web`-bruk.
- **Test feilar lokalt**: køyr `./bin/nc test` og deretter relevant enkeltfil med `run`.
- **Selfhost-status er uklar**: køyr `./bin/nc maintenance verify`.

## Kva du ikkje skal bruke som normalveg

I normal utvikling skal du ikkje innføre:

- nye Python-verktøy for kompilering eller køyring
- nye C-VM-steg i CI
- `ncb_to_c` i produksjonsflyt
- `.py`, `.c` eller `.h` i aktiv `tools/`, `selfhost/`, `bin/`, `bootstrap/` eller CI-flate

Historiske spor høyrer heime i `archive/`.

## Rask sjekkliste

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
