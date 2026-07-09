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
- ELF stage-0-kandidat på Linux x86_64, som byggjer Gen1-ELF og lastar han opp som artifact
- samla driftsvakt via `./bin/nc selfhost-drift-guard`

## Bru-policy

Norscode eig verktøylogikken i `.no`-filene. Shell-filene under `tools/` skal vere avgrensa
kompatibilitetslag for CI, plattformdeteksjon og kommandoar der stage0-seeden manglar
direkte binding, særleg prosesskøyring. Dei skal ikkje gjere portar grøne ved å hoppe over
arbeid; når dei fell tilbake frå `.no`, skal dei køyre den same konkrete verifikasjonen i
shell og feile på reelle avvik.

Alle aktive `.sh`- og `.ps1`-bruer skal anten ha ein `Norscode-first`-markør/bridge som
peikar på `.no`-eigaren, eller vere eit eksplisitt dokumentert unntak. `./bin/nc
surface-ownership` handhevar dette saman med kravet om same-namn `.no`-eigar for
aktive `.sh`, `.ps1`, `.js` og `.swift`-filer.

`./bin/nc selfcompile-stage0-elf` har to nivå:

- Standard CI-nivå byggjer og verifiserer Gen1 stage-0-ELF-kandidaten.
- Djupt ELF-til-ELF-løp kan køyrast manuelt med `NC_OM6B_RUN_STAGE0=1`, men er ikkje ein
  obligatorisk grøn gate før den native ELF-runtimeflata kan køyre kall, miljø og filskriving
  for `selfhost.elf_compile_driver.start`.

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
