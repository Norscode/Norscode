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
- [x] password hashing / secrets-håndtering
- [x] CSRF-beskyttelse der det trengs
- [x] CORS-konfigurasjon som standard
- [x] secure cookies og cookie helpers
- [x] rate limiting og brute-force-beskyttelse
- [x] input-sanitizing og sikkerhetshelpers for vanlige webangrep

## 4. Data og persistens

- [x] databaseadapter for minst én primær database
- [x] migreringsverktøy som standard
- [x] transaksjoner som standard
- [x] connection pooling
- [x] repository- eller modelmønstre som er dokumentert og anbefalt
- [x] enkel og tydelig JSON-/schema-mapping
- [x] fil- og objektlagring som standardmønster
- [x] cache-støtte eller cache-adapter

## 5. Observability og produksjonsdiagnostikk

- [x] strukturert logging
- [x] request-id gjennom hele kjeden
- [x] metrics / counters / histogrammer
- [x] tracing eller span-støtte
- [x] tydelige runtime-feil med kontekst og kallstakk
- [x] audit-/security-logger for sensitive hendelser
- [x] ferdige dashboard-/exporter-mønstre

## 6. API-kontrakt og dokumentasjon

- [x] OpenAPI JSON
- [x] enkel docs-side
- [x] examples for websporet
- [x] automatisk schema fra request- og response-typer i full bredde
- [x] autentisering dokumentert i OpenAPI
- [x] error responses dokumentert og eksemplifisert
- [x] versjonering av API-kontrakter
- [x] migreringsnotater mellom API-versjoner

## 7. Testing og kvalitet

- [x] testkommando med stabil output
- [x] smoke, bench og fuzz
- [x] CI med parity- og selfhost-sjekker
- [x] web-tester for request/response, validation, dependency, middleware og OpenAPI
- [x] integrasjonstester mot ekte database
- [x] end-to-end tester av serveradapter i flere miljøer
- [x] produksjonsnære stresstester
- [x] sikkerhetstester for auth og input

## 8. Plattform og distribusjon

- [x] Windows-installasjon
- [x] binary-first releaseflyt
- [x] eksplisitt bootstrap-verktøy
- [x] container-/Docker-basert distribusjonsoppsett
- [x] systemd- eller service-oppsett
- [x] enkel deployflyt til typiske plattformer
- [x] rollback som standardisert operasjon
- [x] release-notater med breaking change-policy

## 9. Developer Experience

- [x] README med startreise og eksempler
- [x] cookbook og eksempelkatalog
- [x] CLI-kontrakt
- [x] scaffold/generator for nye API-prosjekter
- [x] tydelig mønster for app-oppsett og avhengigheter
- [x] dokumentert folderstruktur for store tjenester
- [x] bedre diagnosekommandoer for drift og feilsøking

## Kort konklusjon

Norscode er allerede sterk på språk, webgrunnmur og dev-flyt.
For å være en full backend må den siste biten særlig dekke:

- [x] produksjonsserver
- [x] auth og sikkerhet
- [x] database og migrering
- [x] observability
- [x] mer moden deploy-/driftshistorikk
