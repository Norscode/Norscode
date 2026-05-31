# Selfhost CI gates

Mål:
Gjøre bootstrap-, fallback- og distribusjonsregresjoner synlige før de når brukere.

## Hvorfor denne gateflaten finnes

Norscode trenger to klare CI-veier:

1. En native bootstrap-lane som verifiserer den ferdige binaryen.
2. En eksplisitt fallback-lane som fortsatt kan brukes når overgangsverktøy må sammenlignes eller sjekkes via den historiske banen.

Begge skal være dokumenterte og enkle å tolke.
Normal CI for release/install skal ikke kreve C-verktøykjede.

## Standard CI-gates

### 1. Bootstrap-lane

Kjør:

```bash
./bin/nc ci --bootstrap-lane
```

Denne veien skal:

- bruke native bootstrap-binary
- verifisere selfhost bootstrap-gaten
- sjekke workflow-policy
- være fri for skjult fallback i normalflyten

### 2. Eksplisitt fallback-lane

Kjør:

```bash
./bin/bootstrap smoke
```

Denne veien skal:

- vise tydelig fallback-varsel
- kjøre CI som eksplisitt overgangs-/diagnoseflyt
- gå via den eksplisitte bootstrap-flaten i `bin/bootstrap`
- brukes for representative install-/run-smoke-sjekker der den historiske banen fortsatt er eksplisitt

## Representative smoke tests

CI bør også ha små, raske tester for:

- installasjon og release
- `doctor`
- `smoke`
- `--version`
- samlet driftsvakt via `tools/selfhost_drift_guard.sh`

## Failure policy

CI skal feile hvis:

- bootstrap-lane feiler
- eksplisitt fallback-lane feiler
- release-/install-smoke feiler
- workflow-policy eller parity-sjekker viser avvik

## Verifikasjon

Praktiske regresjonstester skal dekke:

- bootstrap-lane er native
- fallback krever eksplisitt flagg
- installasjon/release er mekanisk verifiserbar
- driftsregresjoner blir fanget av en liten, eksplisitt guard
