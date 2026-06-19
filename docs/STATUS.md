# Dokumentasjonsstatus

Dette viser status for dokumentasjonen som faktisk ligg i repoet.

## Gjeldande struktur

- `docs/05-development/`
- `docs/_archive/`
- `docs/assets/`

## Aktiv inngang

- [docs/INDEX.md](INDEX.md)
- [docs/USER_MANUAL.md](USER_MANUAL.md)
- [docs/LEARNING_GUIDE.md](LEARNING_GUIDE.md)
- [docs/DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)
- [docs/LANE_MAP.md](LANE_MAP.md)
- [docs/_archive/ARCHIVE_INDEX.md](_archive/ARCHIVE_INDEX.md)

## Vedlikehald

- Normal løype: `./bin/nc run`, `./bin/nc check`, `./bin/nc test`
- Vedlikehald: `./bin/nc maintenance`
- Utvikling av nye funksjonar: `./bin/nc feature-check [fil.no ...]`
- `docs/SELFHOST_HANDLINGSPLAN.md` er aktiv plan for normalflata
- `./bin/nc maintenance status|lane|seed|seed-status|verify|report|report-json` er statusflate i Norscode
- `stage0_seed_ok` er hovudindikatoren for stage-0 seed i `maintenance`-rapportane
- historiske filer skal liggje i `docs/_archive/` eller `archive/`

## Merknad

Gamle status-tal og gamle faser vart skrivne for ein eldre struktur. Dei er no tona ned for å unngå å påstå meir enn det dokumentasjonen faktisk viser.

Historiske release-notat under `.github/releases/` kan framleis vise tal som `27/35` og `30-40%`, men dei er arkivtekst frå publiseringstidspunktet og skal ikkje lesast som dagens status.

## Status for FastAPI-paritet i scaffold (fase 1)

- **Dato:** 2026-06-18
- **Omfang:** `tools/startproject.sh` og `tools/startapp.sh`

### Fullført

- Leggja til responsskjema-validering (`response_shape_validate_or_error`) i scaffolds for testbare API-rykter.
- Lagt inn ny prosjekt-endepunkt:
  - `GET /api/v1/response-model`
- Lagt inn tilsvarande app-endepunkt:
  - `GET /api/v1/${APP_NAME}/response-model`
- Ruteopplisting oppdatert i begge malane (alle `startproject`-variantar med/utan auth/admin).
- Leggje til testdekning for response-model i begge malane:
  - positivt svar (`200`)
  - negativt svar (`500`, `ResponseValidationError`)
- Dokumentasjon i malar:
  - `startproject` README-liste oppdaterte API-ruter
  - `startapp` README-liste oppdaterte API-ruter
- OpenAPI-sjekk i testane oppdatert til å forvente `response-model` i spec.
- Nytt eksempelsett i payload-filer:
  - prosjekt: `tests/payloads/api_payload.json`, `tests/payloads/api_nested.json`
  - app: `apps/<app>/tests/payloads/${APP_NAME}_payload.json`, `apps/<app>/tests/payloads/${APP_NAME}_nested.json`
- Dependency-injeksjon i app-skal:
  - Lagt til `GET /api/v1/${APP_NAME}/dependency` i `startapp` med `app_meta`-dependency.
  - Oppdatert app-ruteopplisting, testdekning og app-README.
- Feilhåndtering i app-skal:
  - Lagt til `GET /api/v1/${APP_NAME}/error` i `startapp`.
  - Demonstrerer standardisert `400` (`response_error`) og `500` (`response_error`), pluss suksess-tilfelle.
  - Oppdatert route-opplisting, testdekning og app-README.
  - Lagt til feilmønster for app-rot med:
    - 404-test for ukjende rute (`GET /api/v1/${APP_NAME}/ikkje-finst`)
    - 405-test for metodemismatch (`POST /api/v1/${APP_NAME}/query`)
- Standardisert request/response-kontrakt i begge skal:
  - Lagt til `POST /api/v1/${APP_NAME}/request-model` i `startapp` med request-validering + response-shape-validering.
  - Lagt til tilsvarande `POST /api/v1/request-model` i `startproject` med tilsvarande valideringsflyt.
  - Oppdatert både app- og stack-route-opplisting, testdekning (suksess + valideringsfeil) og README-rutelister.
- Auth-mønster i app-skal:
  - Lagt til genererte auth-endepunkt i `startapp` (login/register/logout/profile):
    - `GET /api/v1/${APP_NAME}/auth/login`
    - `POST /api/v1/${APP_NAME}/auth/login`
    - `GET /api/v1/${APP_NAME}/auth/register`
    - `POST /api/v1/${APP_NAME}/auth/register`
    - `POST /api/v1/${APP_NAME}/auth/logout`
    - `GET /api/v1/${APP_NAME}/profile`
  - Oppdatert app-ruteopplisting og testdekning for autentiseringsflyt (suksess, ugyldig pålogging, utlogging, profil-metadata, token-mangel).

### Under arbeid / neste steg

- Neste steg i FastAPI-paritet:
  - Oppdatere dokumentasjonen med kvar gjenværande manglande del i FastAPI-liknande standardflow (dependency, middleware, autentiseringsflow med header/session, openapi-schema).
