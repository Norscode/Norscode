# Deployment Playbook

Dette er den praktiske flyten for å ta Norscode fra release til drift.

## Standard rekkefølge

1. Bygg releasepakke med `tools/package_release.no`.
2. Verifiser checksum og installer med `tools/install-release.no`.
3. Pek driftflaten mot `current`.
4. Velg enten `systemd` eller container avhengig av miljø.
5. Bruk `norcode serve --production` som runtime-kommando i begge tilfeller.

Se også:

- [`docs/SELFHOST_RELEASE_CHECKLIST.md`](./SELFHOST_RELEASE_CHECKLIST.md)

## Lokalt eller på egen server

Bruk release + install:

```bash
NORSCODE_RELEASE_VERSION=1.0.0 NORSCODE_ENABLE_EXEC_PROSESS=1 ./bin/nc run tools/package_release.no
NORSCODE_INSTALL_ARCHIVE=release-artifacts/norscode-language-1.0.0.tar.gz NORSCODE_INSTALL_PREFIX=/srv/norscode ./bin/nc run tools/install-release.no
```

Deretter:

- `sudo cp deploy/norscode.service /etc/systemd/system/norscode.service`
- `sudo systemctl daemon-reload`
- `sudo systemctl enable --now norscode`

## Container

Bygg image og kjør appen fra arbeidsmappen:

```bash
docker build -t norscode .
docker run --rm -p 8000:8000 -v "$PWD:/work" norscode
```

## Rollback

- Gå tilbake til forrige release ved å peke `current`-lenken tilbake til en tidligere installasjon.
- Restart tjenesten eller containeren etterpå.
- Bruk release-notatene til å se om endringen krever migrering.

## Standardisert rollback

Hvis du bruker lokal installasjon:

```bash
ls -1 /srv/norscode/releases
ln -sfn /srv/norscode/releases/norscode-language-1.0.0 /srv/norscode/current
sudo systemctl restart norscode
```

Hvis du bruker container:

```bash
docker tag norscode norscode:previous
docker run --rm -p 8000:8000 -v "$PWD:/work" norscode:previous
```

Rollback skal være den samme operasjonen hver gang: velg forrige stabile versjon, pek trafikken dit, og bekreft med `serve-e2e` eller `stress`.

## Hva som skal være sant

- Normal drift skal ikke kreve historisk vei.
- Rollback skal være en vanlig operasjon, ikke en manuell redningsaksjon.
- Deploy skal følge samme mønster hver gang.
