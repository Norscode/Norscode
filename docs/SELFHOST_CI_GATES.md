# Selfhost CI gates

Mål:
Gjøre native-, maintainer- og distribusjonsregresjoner synlige før de når brukere.

## Hvorfor denne gateflaten finnes

Norscode trenger to klare, ulike kontrollflater:

1. En native bootstrap-lane som verifiserer den ferdige binaryen.
2. En eksplisitt maintainer-lane som berre verifiserer den historiske seed-brua når vedlikehald krev det.

Begge skal vere dokumenterte og enkle aa tolke, men berre den første er normal produktflyt.
Normal CI for release/install skal ikke kreve C-verktøykjede.
`bin/bootstrap` skal heller ikkje vere naudsynt i normal release-/install-flyt.

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
- vere fri for skjult fallback i normalflyten

### 2. Eksplisitt maintainer-lane

Kjør:

```bash
bash tools/selfhost_maintenance_verify.sh
```

Denne veien skal:

- vere tydeleg merkt som vedlikehald
- verifisere seed-/generated-C-brua isolert fraa normalflyten
- aldri framstaa som naudsynleg for vanleg release, installasjon eller dagleg utvikling
- berre brukast naar maintainers faktisk jobbar med seed-fornying eller historisk parity

## Representative smoke tests

CI bør også ha små, raske tester for:

- installasjon og release
- `--version`
- ELF self-compile parity på Linux x86_64 som hard gate med `NC_OM6B_RUN_STAGE0=1`
- samlet driftsvakt via `tools/selfhost_drift_guard.sh`

## Failure policy

CI skal feile hvis:

- bootstrap-lane feiler
- eksplisitt maintainer-lane feiler naar ein maintainer-workflow køyrer
- release-/install-smoke feiler
- workflow-policy eller parity-sjekker viser avvik
- ELF self-compile parity viser avvik

## Verifikasjon

Praktiske regresjonstester skal dekke:

- bootstrap-lane er native
- maintainer-brua er eksplisitt og isolert
- installasjon/release er mekanisk verifiserbar
- driftsregresjoner blir fanget av en liten, eksplisitt guard
