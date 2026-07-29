# Linux-app gap-status

Dette er den korte sanninga om kor Linux-appsporet staar no.

## Ferdig

- Linux native runtime og CI-grunnmur
- `AppDir`-layout
- `AppDir.tar.gz` med checksum
- lokal desktop entry og ikoninstallasjon
- repeterbar AppDir-installasjon med versjonert layout
- eigen CI-workflow for Linux app-artefakt

## Delvis

- `AppImage`
  - skript og workflow prøver det
  - manglar verifisert grøn køyring med `appimagetool` i Linux-miljø

- Linux app-distribusjon
  - primær artefakt finst
  - manglar enno distro-spesifikke pakkar

## Manglar

- stabilt verifisert `AppImage`
- eventuelt `linux-arm64` app-pakkeløype
- eventuell signering/attestering
- eventuelle `deb`/`rpm`-spor

## Anbefalt primær distribusjon no

Bruk:

1. `AppDir.tar.gz` som primær, verifisert Linux app-artefakt
2. `AppImage` berre som tillegg når Linux CI/host faktisk byggjer det grønt

Ikkje gjer `AppImage` til einaste sannhet før det er bevist grønt i praksis.
