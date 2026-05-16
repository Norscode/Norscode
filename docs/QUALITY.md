# Quality Checks

Dette dokumentet beskriver de første driftklare kvalitetssjekkene for Norscode.

## Benchmark-suite

Kjør:

```bash
norcode bench
```

Måler faste representative flater:

- `check-map`
- `test-json`
- `test-selfhost`
- `commands-json`

Hver måling har et generøst tidsbudsjett. Budsjettbrudd rapporteres som feil, og `--json` gir maskinlesbar output for CI eller lokal sammenligning.

## Smoke-test

Kjør:

```bash
norcode smoke
```

Smoke-testen gjør en fresh release-løkke:

- bygger bootstrap-binary
- pakker release
- installerer release i en temp-prefix
- kjører `nc --help`
- kjører full `nc test`

Dette er den enkleste måten å verifisere fresh install og fresh release på én gang.

## Serveradapter-e2e

Kjør:

```bash
norcode serve-e2e
```

Denne kontrollen starter `norcode serve` i flere moduser og verifiserer at serveradapteren svarer riktig i:

- normal route-kjøring
- production-modus
- reverse proxy-modus

Bruk denne når du vil sjekke at serverflyten faktisk fungerer som et helhetlig runtime-oppsett, ikke bare som en enkel smoke-test.

## Produksjonsnær stress

Kjør:

```bash
norcode stress
```

Stress-testen sender mange samtidige forespørsler mot en produksjonskonfigurert server og verifiserer at route- og health-flater holder seg stabile under belastning.

Dette er den riktige kontrollen når du vil ha en liten, lokal lasttest uten å trekke inn ekstern infrastruktur.

## Sikkerhetssjekk

Kjør:

```bash
norcode security
```

Denne kontrollen samler de viktigste auth- og input-testene:

- bearer auth
- CSRF
- HTML-escaping og sikre filnavn/slugs
- cookies og guards

Bruk den når du vil ha en enkel, samlet sikkerhetskontroll før release eller deploy.

## Dashboard- og exporter-mønster

Bruk:

```bash
norcode diagnose
```

og [docs/DASHBOARD_EXPORTER_PATTERN.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/DASHBOARD_EXPORTER_PATTERN.md) når du vil koble observability-data til dashboards eller eksportere dem videre til eksterne systemer.

Mønsteret samler:

- `std.metrics`
- `std.log`
- `std.trace`
- `std.audit`

## Diagnose

Kjør:

```bash
norcode diagnose
```

Denne kontrollen gir en samlet status for prosjektet:

- prosjektrot og konfig
- stdlib-oppslag
- avhengigheter
- testdiscovery
- git-status og revision

Bruk den når du vil feilsøke et nytt prosjekt eller få en rask driftssjekk av arbeidskopien.

## Negativt korpus

Kjør:

```bash
norcode fuzz
```

Korpuset verifiserer at parseren feiler kontrollert på noen få representative ugyldige snutter, og at en ubearbeidet runtime-feil blir fanget som forventet.

## Tolkning

- Bruk `bench` for ytelsesovervåking.
- Bruk `smoke` for release- og installasjonsverifisering.
- Bruk `serve-e2e` for serveradapteren i flere miljøer.
- Bruk `stress` for produksjonsnær last og samtidighet.
- Bruk `security` for auth- og inputbeskyttelse.
- Bruk `diagnose` for rask status og feilsøking av prosjektet.
- Bruk `ci` for full kontrollflate når du vil ha alt på en gang.
