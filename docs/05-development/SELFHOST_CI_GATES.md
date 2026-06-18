# Selfhost CI Gates

Målet er å fange regresjonar før dei når brukarar, utan å blande inn legacy-vegar i normal CI.

## Kva som gjeld

- Normal CI skal bruke native CLI og native verifisering
- Normal release og installasjon skal ikkje krevje C-verktøykjede
- Normal CI for release/install skal ikkje krevje C-verktøykjede
- Vedlikehaldsløypa skal vere eksplisitt og tydeleg markert
- `bin/bootstrap` og generert C er berre for historisk vedlikehald

## Normal gate

Køyr:

```bash
./bin/nc selfhost-bootstrap-gate
```

Denne kontrollen skal:

- bruke native bootstrap-binær
- verifisere selfhost bootstrap-gaten
- sjå til at policyen held
- ikkje vere avhengig av skjulte fallback-steg

## Vedlikehaldsløype

Køyr:

```bash
bash tools/selfhost_maintenance_verify.sh
```

Denne løypa skal:

- vere tydeleg merkt som vedlikehald og historikk
- verifisere seed-/generert-C-brua isolert frå normalflyten
- aldri framstå som nødvendig for vanleg release, installasjon eller dagleg utvikling

## Rask kontroll

CI bør òg ha små, raske testar for:

- installasjon og release
- `--version`
- ELF self-compile-paritet på Linux x86_64 som hard gate med `NC_OM6B_RUN_STAGE0=1`
- samla driftsvakt via `tools/selfhost_drift_guard.sh`

For full CI-sjekk:

```bash
./bin/nc ci
```

## Feilreglar

CI skal feile dersom:

- normal bootstrap-gate feiler
- vedlikehaldsløypa feiler når vedlikehald køyrer
- release-/installasjonstest feiler
- workflow-policy eller paritetsjekkar viser avvik
- ELF self-compile-paritet viser avvik

## Verifikasjon

Praktiske regresjonstestar skal dekke at:

- bootstrap-løypa er native
- vedlikehaldsbrua er eksplisitt og isolert
- installasjon og release er mekanisk verifiserbare
- driftsregresjonar blir fanga av ei lita, eksplisitt vakt
