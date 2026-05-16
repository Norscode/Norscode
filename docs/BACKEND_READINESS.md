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

- [x] ekte serveradapter for produksjonstrafikk
- [x] autentisering og autorisasjon som standardmønster
- [x] middleware-kjede
- [x] livssyklus-hooks for oppstart og nedstenging
- [x] cookies og session-hjelpere
- [x] input-sanitizing og sikkerhetshelpers for vanlige webangrep
- [x] database-/persistenskonvensjoner
- [x] cache-adapter for små in-memory verdier
- [x] observability med logger, metrics og tracing
- [x] mer komplett OpenAPI/Swagger-generering

## Mangler før jeg ville kalt det fullverdig backend

- [x] en stabil HTTP-server som kan driftes uten å starte via utviklerverktøy
- [x] tydelige mønstre for auth, rolle- og tilgansstyring
- [x] request validation som føles like automatisk som i etablerte webrammeverk
- [x] første klasses støtte for middleware og sikkerhetshjelpere
- [x] rate limiting og brute-force-beskyttelse
- [x] migrering og databaseoppsett som standardisert del av produktet
- [x] transaksjoner som standard
- [x] connection pooling
- [x] repository- eller modelmønstre som er dokumentert og anbefalt
- [x] enkel og tydelig JSON-/schema-mapping
- [x] fil- og objektlagring som standardmønster
- [x] observability og produksjonsdiagnostikk som standard
- [x] dokumentasjon som viser en komplett backend-app fra første request til deploy

## Praktisk konklusjon

- [x] Norscode er backend-klar for mindre og mellomstore tjenester
- [x] Norscode er ennå ikke en helt moden backend-plattform på nivå med de største rammeverkene
