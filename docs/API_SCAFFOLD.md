# API Scaffold

Bruk `norcode scaffold-api` når du vil starte et nytt API-prosjekt med en standardisert, liten prosjektstruktur.

## Opprett et nytt prosjekt

```bash
norcode scaffold-api min-api
```

Med `--name` kan du overstyre prosjektnavnet som brukes i `norcode.toml` og genererte filer:

```bash
norcode scaffold-api /path/to/new-app --name butikk_api
```

## Hva som genereres

Generatoren lager en enkel base med:

- `app.no` med en liten webflate
- `norcode.toml` med `[project]` og `entry = "app.no"`
- `tests/test_app.no` med første regresjonstest og eksplisitt `bruk app`
- `examples/ping.no` som viser hvordan appen kan kjøres og importeres
- `deploy/norscode.service` som et Linux-serviceutgangspunkt
- `README.md` med neste steg og kjørekommandoer

## Anbefalt struktur

```text
min-api/
├── app.no
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
