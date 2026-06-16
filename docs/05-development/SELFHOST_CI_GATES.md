# Selfhost CI gates

Mål:
Gjøre native- og distribusjonsregresjoner synlige før de når brukere, medan eventuelle historiske vedlikehaldsvegar er tydeleg isolerte.

## Hvorfor denne gateflaten finnes

Norscode bruker i praksis ein normal kontrollflate og ein historisk vedlikehaldsflate:

1. En native bootstrap-lane som verifiserer den ferdige binaryen.
2. Ein eksplisitt historisk vedlikehaldslane som berre skal brukast når det faktisk krevst.

Berre den første er normal produktflyt.
Normal CI for release/install skal ikkje krevje C-verktøykjede, og `bin/bootstrap` skal ikkje vere naudsynt i normal release-/install-flyt.

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

### 2. Historisk vedlikehaldslane

Kjør:

```bash
    bash tools/selfhost_maintenance_verify.sh
```

Denne veien skal:

    - vere tydeleg merkt som vedlikehald og historikk
    - verifisere seed-/generated-C-brua isolert frå normalflyten
    - aldri framstå som naudsynleg for vanleg release, installasjon eller dagleg utvikling
    - berre brukast når nokon faktisk jobbar med seed-fornying eller historisk parity

## Representative smoke tests

CI bør også ha små, raske tester for:

- installasjon og release
- `--version`
- ELF self-compile parity på Linux x86_64 som hard gate med `NC_OM6B_RUN_STAGE0=1`
- samlet driftsvakt via `tools/selfhost_drift_guard.sh`

## Failure policy

CI skal feile hvis:

- bootstrap-lane feiler
- historisk maintainer-lane feiler når ein vedlikehaldsworkflow køyrer
- release-/install-smoke feiler
- workflow-policy eller parity-sjekker viser avvik
- ELF self-compile parity viser avvik

## Verifikasjon

Praktiske regresjonstester skal dekke:

- bootstrap-lane er native
- vedlikehaldsbrua er eksplisitt og isolert
- installasjon/release er mekanisk verifiserbar
- driftsregresjoner blir fanget av en liten, eksplisitt guard
