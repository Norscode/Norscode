# users

Appmodul oppretta av `nc startapp`.

Filer i denne appen:

- `users.no` — modul-API.
- `views.no` — handteringsfunksjonar.
- `forms.no` — form- og JSON-parsing.
- `models.no` — modeldefinisjonar og migrasjonar.
- `routes.no` — app-ruter.
- `dispatcher.no` — app-dispatcher med ruteoppløysing.
- `tests/` — testmalar.
- `tests/payloads/` — API-eksempelpayload.
  - Ekstra API-ruter i malen:
  - `/api/v1/${APP_NAME}/meta`
  - `/api/v1/${APP_NAME}/dependency`
  - `/api/v1/${APP_NAME}/error`
  - `/api/v1/${APP_NAME}/auth/login`
  - `/api/v1/${APP_NAME}/auth/register`
  - `/api/v1/${APP_NAME}/auth/logout`
  - `/api/v1/${APP_NAME}/profile`
  - `/api/v1/${APP_NAME}/query`
  - `/api/v1/${APP_NAME}/validate`
  - `/api/v1/${APP_NAME}/items/{id}`
  - `/api/v1/${APP_NAME}/payload`
  - `/api/v1/${APP_NAME}/nested`
  - `/api/v1/${APP_NAME}/headers`
  - `/api/v1/${APP_NAME}/response-model`
  - `/api/v1/${APP_NAME}/request-model`
