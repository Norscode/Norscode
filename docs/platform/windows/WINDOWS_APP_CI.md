# Windows-app CI

Dette dokumentet skildrar den første CI-linja for Windows app-sporet.

## Workflow

Repoet har no ein eigen workflow:

- `.github/workflows/windows-app-release.yml`

Han gjer følgjande på `windows-latest`:

1. køyrer aktiv-flate-gater
2. sikrar stage-0-seed
3. leitar etter ein eksisterande `norscode.exe`
4. pakkar ZIP-artefakt via `tools/package-windows-app.no`
5. lastar opp ZIP og checksum
6. publiserer artefakta til GitHub Release ved tag

## Triggerar

- `workflow_dispatch`
- tag-push `v*.*.*`

## Sanning først

Denne workflowen er bygd i første praktiske form, men framleis delvis.

Det som er klart:

- release-kontrakt
- ZIP-pakking
- artefakt-opplasting
- GitHub Release-publisering

Det som framleis manglar:

- repeterbar produksjon av sjølve `norscode.exe`

Workflowen stoppar derfor med ein eksplisitt feil dersom han ikkje finn ein Windows-native `.exe` i ein av desse stigane:

- `dist/windows/norscode.exe`
- `build/windows/norscode.exe`
- `release-artifacts/norscode.exe`

## Verifisert primærflate

Det som er den faktiske, verifiserte primærflata no:

- `norscode-windows-<versjon>.zip`
- `norscode-windows-<versjon>.zip.sha256`

## Avgrensing

Denne workflowen dekkjer ikkje enno:

- automatisk Windows-native `.exe`-bygging
- MSI
- code signing
- SmartScreen-/trust-finish
