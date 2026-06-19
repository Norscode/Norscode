# Norscode Lane Map

Dette dokumentet forklarer kva løype du skal bruke for ulike typar arbeid.

## Normal Brukerløype

Bruk denne når du skriv eller køyrer Norscode:

```bash
./bin/nc run app.no
./bin/nc check app.no
./bin/nc test
```

Denne løypa skal vere fri for Python og C.

## Ny Funksjonalitet

Bruk denne når du legg til eller endrar Norscode-funksjonar:

```bash
./bin/nc feature-check app.no
./bin/nc test
```

Ved større endringar:

```bash
./bin/nc selfhost-bootstrap-gate
./bin/nc verify-selvstendighet
```

## Server

Bruk denne når du køyrer web/server:

```bash
./bin/nc serve app.no --port 8080
```

Sjekk først:

```bash
./bin/nc check app.no
```

## NorsDB

Bruk denne for database-røyk-test:

```bash
./bin/nc run NorsDB/norsdb_smoke.nors
```

Ny databasefunksjonalitet skal halde seg til Norscode- og standardbibliotekflater.

## Vedlikehald

Bruk denne for status og rapport:

```bash
./bin/nc maintenance status
./bin/nc maintenance verify
./bin/nc maintenance report-json
```

## Release Og CI

Bruk desse dokumenta:

- [CI-gates](05-development/SELFHOST_CI_GATES.md)
- [Release-sjekkliste](05-development/SELFHOST_RELEASE_CHECKLIST.md)

Vanleg CI-gate:

```bash
./bin/nc ci
```

## Legacy

Historiske C- og Python-spor høyrer til i `archive/`. Dei skal ikkje brukast som normal utviklings-, bygg-, release- eller CI-løype.
