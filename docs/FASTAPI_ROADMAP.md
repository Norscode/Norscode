# FastAPI Roadmap

Dette er en egen roadmap for en FastAPI-lignende webflate oppå Norscode.
Den bygger på det som allerede finnes i `std.http`, `std.web`, async og JSON.

## Mål

- gjøre det lett å definere HTTP-API-er i Norscode
- beholde Norscode-stil, ikke kopiere Python 1:1
- starte smalt og utvide i tydelige trinn

## Hva som allerede finnes

- [x] `std.http` for klient-HTTP og response-hjelpere
- [x] `std.web` for path-matching, dispatch og path-parametre
- [x] async/await-grunnmur
- [x] JSON-støtte
- [x] strukturert feilhåndtering

## Etappe 1: Request og response

Hvorfor:
En web-ramme trenger en tydelig request- og response-modell før routing og handlers blir ryddig.

Leveranse:

- [x] `request_context` som samler metode, path, query, headers og body
- [x] `response_builder` for status, headers og body
- [x] helper for tekstrespons
- [x] helper for JSON-respons
- [x] helper for filrespons
- [x] helper for å hente path-parametre og query-parametre
- [x] enkel feilrespons med standard JSON-form

Ferdig når:

- [x] en handler kan lese requestdata uten å kjenne rå HTTP-objekter
- [x] en handler kan returnere response uten å håndkode wire-format

## Etappe 2: Route handlers

Hvorfor:
Brukere må kunne knytte en metode + path til en funksjon som faktisk håndterer requesten.

Leveranse:

- [x] registrere handler for metode + path
- [x] støtte for path-parametre i handler
- [x] støtte for statisk og parametrisk route-matching
- [x] klar prioritet mellom eksakt match og parametermatch
- [x] 404 og 405 som standardfeil

Ferdig når:

- [x] en liten API-app kan skrives uten manuell dispatch i hver funksjon

## Etappe 3: Validering

Hvorfor:
Det som gjør FastAPI nyttig er ikke bare routing, men at input blir kontrollert og forklart tydelig.

Leveranse:

- [x] query-validering
- [x] body-validering for JSON
- [x] typekonvertering for path-parametre
- [x] lesbare valideringsfeil
- [x] støtte for obligatoriske og valgfrie felt

Ferdig når:

- [x] feil i input blir forklart med felt, forventet type og faktisk verdi

## Etappe 4: Dependency injection

Hvorfor:
Dette er en stor del av FastAPI-opplevelsen og gjør handlers enklere å gjenbruke.

Leveranse:

- [x] eksplisitt dependency-registrering
- [x] automatisk innsetting av request-scoped verdier
- [x] støtte for enkle provider-funksjoner
- [x] støtte for konfigurasjon, auth og database-hjelpere
- [x] deterministisk rekkefølge for dependency-løsning

Ferdig når:

- [x] handlers kan holde seg små fordi felles oppsett injiseres automatisk

## Etappe 5: OpenAPI og docs

Hvorfor:
Et moderne API trenger dokumentasjon som følger koden.

Leveranse:

- [ ] OpenAPI JSON
- [ ] enkel Swagger/Docs-side
- [ ] schema-generering fra handler-signaturer
- [ ] dokumentasjon av path, query og response typer
- [ ] examples per route

Ferdig når:

- [ ] API-et kan utforskes uten å lese kildekoden

## Etappe 6: Middleware og hooks

Hvorfor:
Mellomvare er nødvendig for logging, auth, timing og cross-cutting concerns.

Leveranse:

- [ ] request-middleware
- [ ] response-middleware
- [ ] error middleware
- [ ] startup/shutdown hooks
- [ ] request-id og logging-kroker

Ferdig når:

- [ ] vanlige tverrgående behov kan løses uten å kopiere kode inn i hver handler

## Etappe 7: Serveradapter

Hvorfor:
Det siste steget er å kjøre appen som en ekte webserver.

Leveranse:

- [ ] lokal dev-server
- [ ] produksjonsvennlig adapter
- [ ] støtte for port og host-konfigurasjon
- [ ] enkel reload i utvikling
- [ ] tydelig exit og feilhåndtering ved oppstart

Ferdig når:

- [ ] en FastAPI-lignende app kan kjøres med én kommando

## Etappe 8: Test og eksempler

Hvorfor:
En webflate uten gode eksempler og tester blir fort vanskelig å stole på.

Leveranse:

- [ ] web-eksempelapp
- [ ] request/response-tester
- [ ] validation-tester
- [ ] middleware-tester
- [ ] OpenAPI snapshot-test

Ferdig når:

- [ ] webflaten er stabil nok til å være et produktområde og ikke bare et eksperiment

## Anbefalt rekkefolge

- [x] Request og response
- [x] Route handlers
- [x] Validering
- [x] Dependency injection
- [ ] OpenAPI og docs
- [ ] Middleware og hooks
- [ ] Serveradapter
- [ ] Test og eksempler

## Kortversjon

FastAPI-sporet er ferdig når:

- [ ] det er lett å skrive en API-app i Norscode
- [ ] routing, validering og docs henger sammen
- [x] handlerne blir små og lesbare
- [ ] serverkjøring er enkel og stabil
