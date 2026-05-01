# Systemd-oppsett

Norscode kan kjøres som en vanlig systemd-tjeneste på Linux ved å peke unit-fila mot den ferdige binaryen og en stabil appfil.

## Eksempel

Filen [deploy/norscode.service](/Users/jansteinar/Projects/language_handoff/projects/language/deploy/norscode.service) er et konkret utgangspunkt.

Den antar:

- at release er installert i `/srv/norscode/current`
- at appen ligger i `/srv/norscode/current/app.no`
- at miljøvariabler kan ligge i `/etc/norscode/norscode.env`

## Installasjon

```bash
sudo cp deploy/norscode.service /etc/systemd/system/norscode.service
sudo systemctl daemon-reload
sudo systemctl enable --now norscode
```

## Drift

- `Restart=on-failure` håndterer enkle krasj.
- `KillSignal=SIGTERM` gir graceful shutdown.
- `TimeoutStopSec=30` gir tjenesten tid til å lukke ned pent.
- `ProtectSystem=strict` og `NoNewPrivileges=true` strammer inn standard sikkerhet.

## Tilpasning

Bytt `WorkingDirectory`, `ExecStart` og `EnvironmentFile` til den layouten du bruker i eget prosjekt.
