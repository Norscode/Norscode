# __PROJECT_NAME__

Dette er eit Django-inspirert Norscode prosjektoppsett med auth, migrations, admin og testmalar.

Struktur:

- `app.no` — hovud app-innsalg.
- `manage.no` — management-kommandoar (`migrate`, `status`).
- `src/` — eigenlogikk (modellar, ruter, visingar og auth).
- `src/route_registry.no` — samlen app-route registry for dispatch.
- `tests/` — testmalar for stack.
- `tests/payloads/` — eksempel JSON-payloadar.
- `apps/` — app-modularisering.
- `docs/` — deploy-historikk og lifecycle-dokumentasjon.
- `innstillingar.toml` — innstillingar.

Kommandoar:

- `cd ${PROJECT_DIR}`
- `./bin/nc check app.no`
- `./bin/nc check manage.no`
- `./bin/nc run manage.no migrate`
- `./bin/nc run manage.no status`
- `./bin/nc run manage.no test`
- `./bin/nc serve app.no --port 8080`
- `./bin/nc startapp users`

Dokumentasjon i prosjektet:

- `docs/lifecycle.md` — deploy-livssyklus frå prep og rollback.
- `docs/deploy-log.md` — manuell deploy-historikk.
- `deploy/norscode.service` — eksempel systemd-service for produksjonskøyring.
- `deploy/` — mappestruktur for operasjonelle deployment-filer.

Test-kommandoar:

- `./bin/nc run manage.no test --db :memory: --fixtures create_product`
  - Laster `tests/payloads/create_product.json` som fixture før test.
  - Testane køyrer via `start()`-metodar i kvar `test_*.no`.

Profil/handsaming av innstillingar:

- Standardprofil blir henta frå `PROFILE` i `innstillingar.toml` (globalt sett `development`).
- Miljøvariabel `NORSCODE_PROFILE` (`development`, `testing`, `production`) kan overstyre profilen ved køyring.
- Spesifikk profil i kode: `config.hent_for_profil("testing")`.
- Overstyring av database for testmiljø: set `DATABASE_URL = ":memory:"` i `[testing]`.
- Køyre med produksjonsprofil:
  - `NORSCODE_PROFILE=production ./bin/nc run app.no`

Auth-API:

- `GET /auth/login`
- `POST /auth/login`
- `GET /auth/register`
- `POST /auth/register`
- `POST /auth/logout`
- `GET /profile`
- `GET /openapi.json`
- `GET /docs`
- `GET /api/v1/health`
- `POST /api/v1/echo`
- `GET /api/v1/dependency`
- `GET /api/v1/query`
- `POST /api/v1/validate`
- `GET /api/v1/items/{id}`
- `POST /api/v1/payload`
- `POST /api/v1/nested`
- `GET /api/v1/headers`
- `GET /api/v1/response-model`
- `POST /api/v1/request-model`
- `GET /static/{sti}` (filer frå `static/`)

Sikkerheit:

- `src/middleware.no` køyrer standard pipeline for security/request-logg-rate-limit.
- `src/security.no` legg til standard security headers på alle svar:
  - `X-Content-Type-Options`
  - `X-XSS-Protection`
  - `Strict-Transport-Security`
  - `X-Frame-Options`
  - `Content-Security-Policy`
- Host-kontroll er aktivert mot `ALLOWED_HOSTS` i `innstillingar.toml`.
- Brute-force-vern på login:
  - `src/auth/throttle.no` tek maks forsøk per IP/brukar innan eit vindauge.
  - Ved for mange feil blir `/auth/login` blokkert med `HTTP 429`.
- Konfigurer med:
  - `BRUTEFORCE_LOGIN_ENABLED`
  - `BRUTEFORCE_LOGIN_MAX`
  - `BRUTEFORCE_LOGIN_WINDOW_SEK`
  - `BRUTEFORCE_LOGIN_KEY` (`ip` eller `brukar`)
