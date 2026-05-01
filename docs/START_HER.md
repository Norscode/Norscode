# Start Her

Dette er den korteste veien inn i Norscode for nye brukere.

## 1. Installer eller bruk binary-first

Normalt bruker du ferdig binary:

```bash
./bin/nc --help
./bin/nc test
```

Hvis du trenger eksplisitt bootstrap:

```bash
./bin/bootstrap --help
python3 main.py --help
```

## 2. Verifiser at alt virker

Kjør de tre vanligste sjekkene:

```bash
./bin/nc test
./bin/nc smoke
./bin/nc bench
```

## 3. Kjør ditt første program

Start med det minste mulige programmet:

```norscode
funksjon start() -> heltall {
    skriv("Hei, Norscode!")
    returner 0
}
```

Kjør det med:

```bash
./bin/nc run min.no
```

## 4. Lær de viktigste kommandoene

- `./bin/nc check fil.no` validerer kode uten å kjore den.
- `./bin/nc format fil.no` formaterer kode.
- `./bin/nc lint fil.no` finner vanlige problemer.
- `./bin/nc test` kjører hele testpakken.
- `./bin/nc commands` viser den stabile CLI-kontrakten.

## 5. Se de beste eksemplene

- [examples/basic.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/basic.no)
- [examples/cli.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/cli.no)
- [examples/http.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/http.no)
- [examples/advanced.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/advanced.no)
- [examples/helpdesk.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/helpdesk.no)

## 6. Når du vil videre

- [docs/COOKBOOK.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/COOKBOOK.md)
- [docs/EXAMPLES.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/EXAMPLES.md)
- [docs/CLI_CONTRACT.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/CLI_CONTRACT.md)
- [docs/QUALITY.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/QUALITY.md)

