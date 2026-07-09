# Dokumentasjonsstatus

Dette viser status for dokumentasjonen som faktisk ligg i repoet.

## Gjeldande struktur

- `docs/05-development/`
- `docs/_archive/`
- `docs/assets/`

## Aktiv inngang

- [Dokumentasjonsinngang](INDEX.md)
- [Brukarmanual](USER_MANUAL.md)
- [Opplæringsguide](LEARNING_GUIDE.md)
- [Dokumentasjonsindeks for vedlikehald](DOCUMENTATION_INDEX.md)
- [Løypekart](LANE_MAP.md)
- [Arkivindeks](_archive/ARCHIVE_INDEX.md)

## Vedlikehald

- Normal løype: `./bin/nc run`, `./bin/nc check`, `./bin/nc test`
- Vedlikehald: `./bin/nc maintenance`
- Utvikling av nye funksjonar: `./bin/nc feature-check [fil.no ...]`
- Lokal release-sjekk: `./bin/nc release-preflight`
- Streng GitHub/release-sjekk: `./bin/nc release-preflight --strict`
- Samla lokal grønnliste: `./bin/nc local-green`
- Samla streng grønnliste: `./bin/nc local-green --strict`
- `docs/SELFHOST_HANDLINGSPLAN.md` er aktiv plan for normalflata
- `./bin/nc maintenance status|lane|seed|seed-status|verify|report|report-json` er statusflate i Norscode
- `stage0_seed_ok` er hovudindikatoren for stage-0 seed i `maintenance`-rapportane
- historiske filer skal liggje i `docs/_archive/` eller `archive/`

## Gjeldande sjølvstendighetsstatus

- `./bin/nc local-green --strict` er lokal bevisport for release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate.
- `./bin/nc test` skal rapportere faktiske testtal, ikkje 0/0 når testfiler finst.
- `./bin/nc release-preflight` skal vere grøn før tag/release og publiserer ingenting.
- `./bin/nc release-preflight --strict` skal vere grøn før push/tag når nye nøkkelfiler skal med til GitHub.
- `./bin/nc local-green` skal vere samla lokal port når release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate må bevisast saman.
- `./bin/nc local-green --strict` skal vere samla streng port før push/tag.
- `./bin/nc selvstendighet` er normal gate for L1-L6/selfhost-status.
- `./bin/nc active-surface` vernar aktiv C/Python-fri flate.
- `./bin/nc surface-ownership` vernar ikkje-Norscode-filer med `.no`-eigarar og krev `Norscode-first`-markør/bridge eller eksplisitt unntak for aktive `.sh`- og `.ps1`-bruer.
- Aktiv plattformkode utanfor Norscode ligg under `platform/` og skal vere dokumentert der.

## Merknad

Gamle status-tal og gamle fasar vart skrivne for ein eldre struktur. Dei er no tona ned for å unngå å påstå meir enn det dokumentasjonen faktisk viser.

Historiske release-notat under `.github/releases/` kan framleis vise tal som `27/35` og `30-40%`, men dei er arkivtekst frå publiseringstidspunktet og skal ikkje lesast som dagens status.

## Status for FastAPI-paritet i scaffold (fase 1)

- **Dato:** 2026-06-18
- **Omfang:** `nc startproject` / `tools/startproject.no` og `nc startapp` / `tools/startapp.no`

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
  - `startproject` README-lister oppdaterte API-ruter
  - `startapp` README-lister oppdaterte API-ruter
- OpenAPI-sjekk i testane oppdatert til å forvente `response-model` i spec.
- Nytt dømesett i payload-filer:
  - prosjekt: `tests/payloads/api_payload.json`, `tests/payloads/api_nested.json`
  - app: `apps/<app>/tests/payloads/${APP_NAME}_payload.json`, `apps/<app>/tests/payloads/${APP_NAME}_nested.json`
- Dependency-injeksjon i app-skal:
  - Lagt til `GET /api/v1/${APP_NAME}/dependency` i `startapp` med `app_meta`-dependency.
  - Oppdatert app-ruteopplisting, testdekning og app-README.
- Feilhåndtering i app-skal:
  - Lagt til `GET /api/v1/${APP_NAME}/error` i `startapp`.
  - Demonstrerer standardisert `400` (`response_error`) og `500` (`response_error`), pluss suksess-tilfelle.
  - Oppdatert ruteopplisting, testdekning og app-README.
  - Lagt til feilmønster for app-rot med:
    - 404-test for ukjende rute (`GET /api/v1/${APP_NAME}/ikkje-finst`)
    - 405-test for metodemismatch (`POST /api/v1/${APP_NAME}/query`)
- Standardisert request/response-kontrakt i begge skal:
  - Lagt til `POST /api/v1/${APP_NAME}/request-model` i `startapp` med request-validering + response-shape-validering.
  - Lagt til tilsvarande `POST /api/v1/request-model` i `startproject` med tilsvarande valideringsflyt.
  - Oppdatert både app- og stack-ruteopplisting, testdekning (suksess + valideringsfeil) og README-rutelister.
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
