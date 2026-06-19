# Norscode Opplæringsguide

Denne guiden lærer deg Norscode steg for steg. Han er laga for deg som vil forstå både språket og arbeidsflyten, ikkje berre kopiere kommandoar.

Bruk repo-rota som arbeidsmappe når du følgjer leksjonane.

## Mål

Når du er ferdig, skal du kunne:

- skrive små Norscode-program
- bruke variablar, typar, funksjonar og kontrollflyt
- lese og skrive enkle datastrukturar
- bruke modular frå `std`
- køyre `check`, `run`, `test` og `feature-check`
- forstå normal selfhost-kjede på eit praktisk nivå

## Før Du Startar

Kontroller miljøet:

```bash
./bin/nc --help
./bin/nc doctor
```

Om dette går grønt, er du klar.

## Leksjon 1: Første Program

Lag `kurs_hei.no`:

```norscode
funksjon start() -> heiltall {
    skriv("Hei frå Norscode")
    returner 0
}
```

Køyr:

```bash
./bin/nc run kurs_hei.no
```

Sjekk:

```bash
./bin/nc check kurs_hei.no
```

Det viktigaste her:

- `funksjon` definerer ein funksjon.
- `start` er vanleg inngangspunkt for små program.
- `skriv(...)` skriv tekst.
- `returner 0` betyr at programmet avsluttar normalt.

## Leksjon 2: Variablar Og Typar

Lag `kurs_typar.no`:

```norscode
funksjon start() -> heiltall {
    la namn: tekst = "Norscode"
    la alder: heiltall = 2
    la aktiv: boolsk = sann

    skriv(namn)
    skriv(alder)
    skriv(aktiv)
    returner 0
}
```

Køyr:

```bash
./bin/nc run kurs_typar.no
```

Vanlege typar:

- `tekst`
- `heiltall`
- `boolsk`
- lister
- ordbøker

Nokre eldre filer bruker variantar som `heltall`. Når du skriv ny dokumentert kode, bruk forma som står i denne guiden.

## Leksjon 3: Funksjonar

Lag `kurs_funksjon.no`:

```norscode
funksjon helsing(namn: tekst) -> tekst {
    returner "Hei " + namn
}

funksjon start() -> heiltall {
    skriv(helsing("Jan"))
    returner 0
}
```

Køyr:

```bash
./bin/nc run kurs_funksjon.no
```

Mønster:

```norscode
funksjon namn(parameter: type) -> returtype {
    returner verdi
}
```

## Leksjon 4: Kontrollflyt

Lag `kurs_kontroll.no`:

```norscode
funksjon start() -> heiltall {
    la tal: heiltall = 3

    hvis tal > 2 {
        skriv("stor nok")
    } ellers {
        skriv("for liten")
    }

    la i: heiltall = 0
    mens i < 3 {
        skriv("runde")
        i = i + 1
    }

    returner 0
}
```

Køyr:

```bash
./bin/nc run kurs_kontroll.no
```

Du bruker:

- `hvis` for val
- `ellers` for alternativ
- `mens` for løkke

## Leksjon 5: Lister Og Ordbøker

Lag `kurs_data.no`:

```norscode
funksjon start() -> heiltall {
    la namn = ["Ada", "Grace", "Linus"]
    skriv(namn[0])

    la person = { "namn": "Norscode", "status": "aktiv" }
    skriv(person["namn"])

    returner 0
}
```

Køyr:

```bash
./bin/nc run kurs_data.no
```

Lister bruker indeks:

```norscode
namn[0]
```

Ordbøker bruker nøkkel:

```norscode
person["namn"]
```

## Leksjon 6: Modular

Lag `kurs_modul.no`:

```norscode
bruk std.json som json

funksjon start() -> heiltall {
    la data = json.parse("{\"namn\":\"Norscode\"}")
    skriv(data["namn"])
    returner 0
}
```

Køyr:

```bash
./bin/nc run kurs_modul.no
```

`bruk std.json som json` betyr:

- hent modulen `std.json`
- bruk namnet `json` i denne fila

## Leksjon 7: JSON Og Data

Lag `kurs_json.no`:

```norscode
bruk std.json som json

funksjon start() -> heiltall {
    la payload = "{\"navn\":\"Norscode\",\"aktiv\":true,\"alder\":2}"
    la parsed = json.parse(payload)

    skriv(parsed["navn"])
    skriv(parsed["aktiv"])
    skriv(json.stringify(parsed))

    returner 0
}
```

Køyr:

```bash
./bin/nc run kurs_json.no
```

Dette er nyttig når du jobbar med API, config, server eller databaseflyt.

## Leksjon 8: Bygg Ein Liten Funksjon

Lag `kurs_score.no`:

```norscode
funksjon status_for_score(score: heiltall) -> tekst {
    hvis score >= 80 {
        returner "klar"
    }
    hvis score >= 50 {
        returner "under arbeid"
    }
    returner "må jobbast med"
}

funksjon start() -> heiltall {
    skriv(status_for_score(86))
    skriv(status_for_score(64))
    skriv(status_for_score(20))
    returner 0
}
```

Køyr:

```bash
./bin/nc run kurs_score.no
./bin/nc check kurs_score.no
./bin/nc feature-check kurs_score.no
```

Dette er den normale utviklingsrytmen:

1. Skriv liten funksjon.
2. Køyr `check`.
3. Køyr `run`.
4. Køyr `feature-check`.
5. Legg til test når funksjonen skal bli del av prosjektet.

## Leksjon 9: Arbeid Med Prosjekt

Opprett prosjekt:

```bash
nc startproject kursprosjekt
```

Gå inn i prosjektet og sjekk fila som vart laga:

```bash
cd kursprosjekt
nc check app.no
nc run app.no
```

Tilbake i repo-rota kan du òg starte server frå ei Norscode-fil:

```bash
./bin/nc serve app.no --port 8080
```

## Leksjon 10: Kva Som Skjer Under Panseret

Når du køyrer:

```bash
./bin/nc run app.no
```

går Norscode normalt gjennom:

```text
app.no
  -> lexer
  -> parser
  -> semantic
  -> bytecode
  -> NCB JSON
  -> selfhost/vm.no
```

Poenget med selfhost-løypa er at ny funksjonalitet skal kunne byggjast og verifiserast direkte i Norscode utan Python/C som normal arbeidsveg.

## Øvingar

1. Lag ein funksjon som tek eit namn og returnerer ei helsing.
2. Lag ei liste med tre namn og skriv ut alle med `mens`.
3. Lag ei ordbok med `tittel`, `status` og `versjon`.
4. Parse ein JSON-streng og skriv ut eitt felt.
5. Køyr `feature-check` på fila di.

## Når Noko Feiler

Bruk denne rekkefølgja:

```bash
./bin/nc check fila.no
./bin/nc run fila.no
./bin/nc feature-check fila.no
./bin/nc doctor
```

Les feilmeldinga frå toppen. Fiks første konkrete syntaks- eller semantikkfeil før du går vidare.

## Neste Steg

Når du kan leksjonane over:

- les [brukermanualen](USER_MANUAL.md)
- les [selfhost handlingsplan](SELFHOST_HANDLINGSPLAN.md)
- sjå på [AST-kontrakten](SELFHOST_PHASE3_AST_CONTRACT_V1.md)
- sjå på [IR og bytecode](SELFHOST_PHASE3_IR_BYTECODE_V1.md)
