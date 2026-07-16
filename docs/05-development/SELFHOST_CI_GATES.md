# Selfhost CI-portar

Målet er å fange regresjonar før dei når brukarar, utan å blande inn historiske vegar i normal CI.

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

## Funksjonsgate

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
- samla driftsvakt via `./bin/nc run tools/selfhost_drift_guard.no`

For full CI-sjekk:

```bash
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green
./bin/nc local-green --strict
./bin/nc ci
```

`release-preflight` er lokal og publiserer ingenting. Han kontrollerer at
release-arbeidsflytane er tag-styrte, at sjekksummar er med, og at release- og
installasjonsflata framleis peikar på Norscode-eigde verktøy.
`release-preflight --strict` er siste lokale port før GitHub/release og feilar
dersom nøkkelfiler finst lokalt utan å vere
spora i git.

`local-green` er òg lokal og publiserer ingenting. Han køyrer release-preflight,
aktiv C/Python-fri flate, fase-0-policy, L1-L6-sjølvstendighet og full testflate
som ei samla grønnliste før tag eller større rydding. Bruk
`./bin/nc local-green --strict` når grønnlista skal vere siste port før push/tag.
Bruk `./bin/nc local-green --list` eller `./bin/nc local-green --strict --list`
for å sjå stega med kommandoar utan å køyre dei.

## Feilreglar

CI skal feile dersom:

- normal bootstrap-gate feilar
- vedlikehaldsløypa feilar når vedlikehald køyrer
- release-/installasjonstest feilar
- arbeidsflyt-policy eller paritetsjekkar viser avvik
- ELF stage-0-kandidaten ikkje kan byggjast

## Verifikasjon

Praktiske regresjonstestar skal dekke at:

- bootstrap-løypa er native
- vedlikehaldsbrua er eksplisitt og isolert
- installasjon og release er mekanisk verifiserbare
- driftsregresjonar blir fanga av ei lita, eksplisitt vakt
