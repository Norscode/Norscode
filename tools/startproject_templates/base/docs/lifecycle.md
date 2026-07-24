# Deploy lifecycle og drift i dette prosjektet

Dette dokumentet beskriv ein enkel deploy-flyt du kan bruke frå dagleg utvikling til produksjon.

## 1) Klargjering før release

- Oppdater avhengigheiter i `innstillingar.toml` ved behov.
- Køyre validering:
  - `./bin/nc check app.no`
  - `./bin/nc check manage.no`
  - `./bin/nc run manage.no migrate`
- Køyre testar:
  - `./bin/nc run manage.no test`
- Valider at migrasjonane er rydda:
  - `./bin/nc run manage.no status`

## 2) Build/prepare

- Vel produksjonsprofil: `NORSCODE_PROFILE=production`.
- Sikre at `deploy/norscode.service` har rett `WorkingDirectory` og `ExecStart`.
- Bygg artefakt i din CI/CD eller pakk repo-state etter ønskjeleg praksis.

## 3) Deploy

- Last opp/oppdater prosjektet på målmiljø.
- Køyre migrasjonar på målmiljø før appstart om nødvendig.
- Start tenesta og sjekk helsestatus.

Standard startkommando:

```sh
./bin/nc run app.no
```

Med systemd:

```sh
systemctl start norscode
systemctl status norscode
```

## 4) Overvaking etter deploy

- Følg loggar frå tjeneste eller runtime.
- Ver observant på autentiserings- og rate-limit-statistikk.
- Sjekk `docs/deploy-log.md` etter kvar deploy med versjons- og rollback-notat.

## 5) Rollback

- Dersom deploy gir regresjon:
  - steng ny versjon,
  - reverter kode eller bytt tilbake siste kjente gode revisjon,
  - køyr migrasjonar som trengst for å rulle tilbake eller justere datastrukturar.
- Logg hendinga i `docs/deploy-log.md` som `rollback`.

## 6) Vedlikehald

- Oppdater `docs/deploy-log.md` ved kvart publiseringssteg.
- Rydd eventuelle `cache`- og temp-filer.
- Gjenta testløpet før neste deploy.

## 7) Oppgåver før neste release

- Fullfør nye migrasjonar i `migrations/`.
- Oppdater `tests/payloads/` og `tests/` ved API-endringar.
- Oppdater denne livssyklusen om flyten endrar seg.
