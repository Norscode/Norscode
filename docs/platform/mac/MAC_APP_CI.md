# Mac-app CI

Dette dokumentet skildrar den første CI-linja for Norscode sin macOS-app.

## Workflow

Repoet har no ein eigen workflow:

- `.github/workflows/macos-app-release.yml`

Han gjer følgjande paa `macos-latest`:

1. køyrer aktiv-flate-gater
2. byggjer `dist/norscode_native`
3. byggjer GUI-varianten av `Norscode.app`
4. ad-hoc-signerer appen, eller bruker Developer ID dersom secrets finst
5. pakkar ZIP/PKG og prøver DMG
6. notariserer PKG dersom Apple-secrets finst
7. lastar opp artefakta

## Triggerar

- `workflow_dispatch`
- tag-push `v*.*.*`

## GitHub Release

Ved tag-push lastar workflowen opp macOS-app-artefakt til GitHub Release.

Per no er dette den faktiske, verifiserte primærflata:

- ZIP
- PKG

DMG blir forsøkt, men er framleis beste innsats inntil den er stabil i CI.

## Avgrensing

Denne workflowen brukar ad-hoc-signering som fallback, men er no klar for Developer ID- og notarization-steg via secrets.

Den aktive appvarianten i workflowen er no GUI-host-sporet, ikkje den eldre Terminal-launcheren. Dersom Terminal-varianten trengst for vedlikehald eller samanlikning, kan han framleis byggjast lokalt med `NORSCODE_MACOS_APP_MODE=terminal ./bin/nc run tools/build-macos-app.no`.

Det som framleis manglar for full produksjonslinje:

- verifisert Developer ID-signering i faktisk CI-miljø
- verifisert notarization i faktisk CI-miljø
- stapling for DMG ved stabil DMG-linje
- eventuelt universal binary
