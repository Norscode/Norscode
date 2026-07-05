# Selfhost CI Gates

Målet er å fange regresjonar før dei når brukarar, utan å blande inn legacy-vegar i normal CI.

## Kva som gjeld

- Normal CI skal bruke native CLI og native verifisering
- Normal release og installasjon skal ikkje krevje C-verktøykjede
- Normal CI for release/install skal ikkje krevje C-verktøykjede
- Aktiv verktøyflate skal ikkje ha C/Python-løype
- Historisk C skal berre liggje under `archive/`

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

## Feature-gate

Køyr:

```bash
./bin/nc feature-check [fil.no ...]
```

Denne løypa skal:

- bruke `dist/norscode_native`
- sjekke aktiv flate utan C/Python
- kompilere/sjekke nye `.no`-filer direkte med Norscode
- falle tilbake til testflata når ingen filer er oppgitt

## Rask kontroll

CI bør òg ha små, raske testar for:

- installasjon og release
- `--version`
- ELF self-compile-paritet på Linux x86_64 som hard gate med `NC_OM6B_RUN_STAGE0=1`
- samla driftsvakt via `tools/selfhost_drift_guard.sh`

## Wrapper-policy

Norscode eig verktøylogikken i `.no`-filene. Shell-filene under `tools/` skal vere tynne
kompatibilitetslag for CI, plattformdeteksjon og kommandoar som stage0-seeden enno ikkje
kan utføre sjølv, særleg prosesskøyring. Dei skal ikkje gjere portar grøne ved å hoppe over
arbeid; når dei fell tilbake frå `.no`, skal dei køyre den same konkrete verifikasjonen i
shell og feile på reelle avvik.

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
