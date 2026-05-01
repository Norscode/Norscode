# Backend Readiness

Denne sjekklisten beskriver hvor nær Norscode er å være en fullverdig backend-plattform i praksis, ikke bare et språk med web-hjelpere.

## Allerede på plass

- [x] `std.http` for klient-HTTP
- [x] `std.web` med path-matching, routing, request/response og filrespons
- [x] query/path/body-validering
- [x] dependency injection
- [x] `async`/`await`
- [x] JSON-støtte
- [x] strukturert feilhåndtering
- [x] test, lint, format, bench, smoke og fuzz
- [x] Windows-installasjon via `pip`
- [x] binary-first release- og installasjonsflyt

## Delvis på plass

- [ ] ekte serveradapter for produksjonstrafikk
- [ ] autentisering og autorisasjon som standardmønster
- [ ] middleware-kjede
- [ ] livssyklus-hooks for oppstart og nedstenging
- [ ] cookies og session-hjelpere
- [ ] database-/persistenskonvensjoner
- [ ] observability med logger, metrics og tracing
- [ ] mer komplett OpenAPI/Swagger-generering

## Mangler før jeg ville kalt det fullverdig backend

- [ ] en stabil HTTP-server som kan driftes uten å starte via utviklerverktøy
- [ ] tydelige mønstre for auth, rolle- og tilgansstyring
- [ ] request validation som føles like automatisk som i etablerte webrammeverk
- [ ] første klasses støtte for middleware, rate limiting og sikkerhetshjelpere
- [ ] migrering og databaseoppsett som standardisert del av produktet
- [ ] observability og produksjonsdiagnostikk som standard
- [ ] dokumentasjon som viser en komplett backend-app fra første request til deploy

## Praktisk konklusjon

- [x] Norscode er backend-klar for mindre og mellomstore tjenester
- [ ] Norscode er ennå ikke en helt moden backend-plattform på nivå med de største rammeverkene

