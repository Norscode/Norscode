# Linux-app sluttrapport

Denne rapporten er den korte sanninga om kva som no gjeld for Linux-appsporet.

## Konklusjon

Linux-sporet er no bygd til eit praktisk første app-/pakkenivå utover rein binær:

- Linux har ein faktisk `AppDir`-layout
- Linux har eit transportabelt app-artefakt i `AppDir.tar.gz`
- Linux har desktop-integrasjon paa brukarflata
- Linux har repeterbar lokal installasjon
- Linux har eigen CI-/release-løype for app-artefakt

## Primær veg

Den eine sikre, verifiserte Linux-vegen no er:

1. bygg `AppDir`
2. pakk `AppDir.tar.gz`
3. installer via `tools/install-linux-appdir.no`

## Sekundær veg

`AppImage` er klargjord som sekundær veg, men er framleis beste innsats inntil han er bevist grønt i Linux-miljø med `appimagetool`.

## Praktisk tolkning

Linux-sporet er funksjonelt og brukbart no.
Det som står att er ikkje grunnleggjande Linux-appstøtte, men sluttpolish:

- bevise `AppImage`
- eventuelt fleire pakkeformat
- eventuelt signering/attestering
