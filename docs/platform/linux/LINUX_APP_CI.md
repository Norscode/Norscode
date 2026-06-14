# Linux-app CI

Dette dokumentet skildrar den første CI-linja for Linux app-sporet.

## Workflow

Repoet har no ein eigen workflow:

- `.github/workflows/linux-app-release.yml`

Han gjer følgjande paa `ubuntu-latest`:

1. køyrer aktiv-flate-gater
2. byggjer `dist/norscode_native`
3. byggjer Linux app-artefakt via `tools/package-linux-app.sh`
4. lastar opp AppDir-tarball
5. prøver AppImage som beste innsats
6. publiserer artefakta til GitHub Release ved tag

## Verifisert primærflate

Det som er den faktiske, verifiserte primærflata no:

- `AppDir`
- `AppDir.tar.gz`

AppImage er framleis beste innsats inntil det er bevist grønt i Linux CI med `appimagetool`.

## Avgrensing

Denne workflowen dekkjer ikkje enno:

- full AppImage-verifikasjon
- distro-spesifikke pakkar som `deb` eller `rpm`
- signering/attestering
