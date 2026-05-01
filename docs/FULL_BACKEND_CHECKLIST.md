# Full Backend Checklist

Dette er en praktisk sjekkliste for når Norscode kan kalles en full backend-plattform i samme klasse som modne webrammeverk.
Jeg bruker den som en sluttliste for produksjonsmoden webdrift, ikke bare som språk- eller demo-støtte.

## 1. Server og drift

- [x] lokal dev-server via `norcode serve`
- [x] produksjonsadapter for langkjørende tjeneste via `norcode serve --production`
- [x] signalstyrt graceful shutdown
- [x] worker-/restart-strategi via `norcode serve --restart-on-crash`
- [x] støtte for reverse proxy-oppsett
- [x] støtte for port og host-konfigurasjon
- [x] støtte for timeouts og keep-alive i produksjon
- [x] tydelig feilrapportering ved oppstart
- [x] health/readiness/liveness-endepunkter som standardmønster

## 2. Routing og requestflyt

- [x] path-matching og route-dispatch
- [x] request_context og response_builder
- [x] route-handlers med metadata
- [x] path-, query- og body-validering
- [x] dependency injection
- [x] middleware og hooks
- [x] subroutere eller router-moduler
- [x] route-guards eller policy-hooks
- [x] standardisert håndtering av HEAD, OPTIONS og metodeforhandling

## 3. Autentisering og sikkerhet

- [x] sessions eller token-basert auth som standardmønster
- [x] rolle- og rettighetsmodell
- [ ] password hashing / secrets-håndtering
- [ ] CSRF-beskyttelse der det trengs
- [ ] CORS-konfigurasjon som standard
- [ ] secure cookies og cookie helpers
- [ ] rate limiting og brute-force-beskyttelse
- [ ] input-sanitizing og sikkerhetshelpers for vanlige webangrep

## 4. Data og persistens

- [ ] databaseadapter for minst én primær database
- [ ] migreringsverktøy som standard
- [ ] transaksjoner og connection pooling
- [ ] repository- eller modelmønstre som er dokumentert og anbefalt
- [ ] enkel og tydelig JSON-/schema-mapping
- [ ] fil- og objektlagring som standardmønster
- [ ] cache-støtte eller cache-adapter

## 5. Observability og produksjonsdiagnostikk

- [ ] strukturert logging
- [ ] request-id gjennom hele kjeden
- [ ] metrics / counters / histogrammer
- [ ] tracing eller span-støtte
- [ ] tydelige runtime-feil med kontekst og kallstakk
- [ ] audit-/security-logger for sensitive hendelser
- [ ] ferdige dashboard-/exporter-mønstre

## 6. API-kontrakt og dokumentasjon

- [x] OpenAPI JSON
- [x] enkel docs-side
- [x] examples for websporet
- [ ] automatisk schema fra request- og response-typer i full bredde
- [ ] autentisering dokumentert i OpenAPI
- [ ] error responses dokumentert og eksemplifisert
- [ ] versjonering av API-kontrakter
- [ ] migreringsnotater mellom API-versjoner

## 7. Testing og kvalitet

- [x] testkommando med stabil output
- [x] smoke, bench og fuzz
- [x] CI med parity- og selfhost-sjekker
- [x] web-tester for request/response, validation, dependency, middleware og OpenAPI
- [ ] integrasjonstester mot ekte database
- [ ] end-to-end tester av serveradapter i flere miljøer
- [ ] produksjonsnære stresstester
- [ ] sikkerhetstester for auth og input

## 8. Plattform og distribusjon

- [x] Windows-installasjon
- [x] binary-first releaseflyt
- [x] eksplisitt bootstrap-verktøy
- [ ] container-/Docker-basert distribusjonsoppsett
- [ ] systemd- eller service-oppsett
- [ ] enkel deployflyt til typiske plattformer
- [ ] rollback som standardisert operasjon
- [ ] release-notater med breaking change-policy

## 9. Developer Experience

- [x] README med startreise og eksempler
- [x] cookbook og eksempelkatalog
- [x] CLI-kontrakt
- [ ] scaffold/generator for nye API-prosjekter
- [ ] tydelig mønster for app-oppsett og avhengigheter
- [ ] dokumentert folderstruktur for store tjenester
- [ ] bedre diagnosekommandoer for drift og feilsøking

## Kort konklusjon

Norscode er allerede sterk på språk, webgrunnmur og dev-flyt.
For å være en full backend må den siste biten særlig dekke:

- [ ] produksjonsserver
- [ ] auth og sikkerhet
- [ ] database og migrering
- [ ] observability
- [ ] mer moden deploy-/driftshistorikk
