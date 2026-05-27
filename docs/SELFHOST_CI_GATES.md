# Selfhost CI gates

Mål:
Gjøre bootstrap-, fallback- og distribusjonsregresjoner synlige før de når brukere.

## Hvorfor denne gateflaten finnes

Norscode trenger to klare CI-veier:

1. En native bootstrap-lane som verifiserer den ferdige binaryen.
2. En eksplisitt fallback-lane som fortsatt kan brukes når overgangsverktøy må sammenlignes eller sjekkes via Python.

Begge skal være dokumenterte og enkle å tolke.

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
- være fri for skjult Python-fallback i normalflyten

### 2. Eksplisitt fallback-lane

Kjør:

```bash
./bin/nc --legacy-python-fallback smoke
```

Denne veien skal:

- vise tydelig fallback-varsel
- kjøre CI som eksplisitt overgangs-/diagnoseflyt
- gå via den gjenværende Python-kompatibiliteten i `norcode/legacy_main.py` og `norcode/bootstrap/python_entry.py`
- brukes for representative install-/run-smoke-sjekker der Python fortsatt er eksplisitt

## Representative smoke tests

CI bør også ha små, raske tester for:

- installasjon og release
- `doctor`
- `smoke`
- `--version`

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
