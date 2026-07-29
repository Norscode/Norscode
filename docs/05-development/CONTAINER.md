# Containeroppsett

Norscode kan pakkes som en Docker-container som bygger den vanlige bootstrap-binaryen og kjører appen via `norcode serve`.

## Bygg

```bash
docker build -t norscode .
```

## Kjør en app fra arbeidsmappen

```bash
docker run --rm -p 8000:8000 -v "$PWD:/work" norscode
```

Standardkommandoen starter eksempelappen i `/work/examples/web_routes.no`. Overstyr `CMD` hvis du vil kjøre en annen `.no`-fil:

```bash
docker run --rm -p 8000:8000 -v "$PWD:/work" norscode serve /work/app.no --host 0.0.0.0 --port 8000 --production
```

## Hva dette gir

- Et image som bygger og pakker Norscode på samme måte som resten av release-flyten.
- En tydelig runtime som kan driftes i container- og orchestrator-miljøer.
- En enkel volum-basert arbeidsflyt for egne backend-prosjekter.
