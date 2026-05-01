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

## Negativt korpus

Kjør:

```bash
norcode fuzz
```

Korpuset verifiserer at parseren feiler kontrollert på noen få representative ugyldige snutter, og at en ubearbeidet runtime-feil blir fanget som forventet.

## Tolkning

- Bruk `bench` for ytelsesovervåking.
- Bruk `smoke` for release- og installasjonsverifisering.
- Bruk `ci` for full kontrollflate når du vil ha alt på en gang.
