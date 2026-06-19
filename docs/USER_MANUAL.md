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
./bin/nc startproject mittprosjekt
./bin/nc startapp minapp
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
- `std.db`: databaseflate brukt av NorsDB og smoke-testar.
- `std.tekst`: tekstoperasjonar.
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

## Prosjekt Og Apper

Opprett prosjekt:

```bash
./bin/nc startproject mittprosjekt
```

Opprett app-modul:

```bash
./bin/nc startapp minapp
```

Etter generering:

```bash
./bin/nc check app.no
./bin/nc run app.no
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
