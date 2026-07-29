# API Scaffold

Bruk `nc startproject` når du vil starte et nytt prosjekt med en standardisert, liten prosjektstruktur.

`nc scaffold-api` er avvikla og er no eit alias for `nc startproject`.

## Opprett et nytt prosjekt

```bash
nc startproject min-api
```

Med `--name` kan du overstyre prosjektnavnet som brukes i `norcode.toml` og genererte filer:

```bash
nc startproject /path/to/new-app --name butikk_api
```

## Hva som genereres

Generatoren lager ein demo med:

- `app.no` med:
  - `GET /api/hello` som returnerer `{"message":"Hello World"}`
  - `GET /admin` som returnerer eit HTML-svar definert i `app.no`
- `norcode.toml` med `[project]` og `entry = "app.no"`
- `src/routes.no` som utvidbar plass for fleire ruter
- `tests/test_app.no` med starttest som verifiserer kjøretidsflyt
- `examples/ping.no` med ei enkel `GET /ping`-route
- `deploy/norscode.service` som et Linux-serviceutgangspunkt
- `README.md` med neste steg og kjørekommandoer
- `app.no` inneheld også HTML for `GET /admin`

## Anbefalt struktur

```text
min-api/
├── app.no
├── src/
│   └── routes.no
├── deploy/
│   └── norscode.service
├── examples/
│   └── ping.no
├── norcode.toml
├── README.md
└── tests/
    └── test_app.no
```

## Hvordan bygge videre

- Legg til flere ruter i `app.no`.
- Utvid testene i `tests/`.
- Tilpass `deploy/norscode.service` til miljøet ditt.
- Bruk `norcode serve app.no --production` for drift.
