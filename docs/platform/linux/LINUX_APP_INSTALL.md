# Linux-app installasjon

Dette dokumentet skildrar den første repeterbare installasjonsløypa for Linux app-sporet.

## Standard flyt

1. bygg artefakt:

```bash
bash tools/package-linux-app.sh --format tarball
```

2. installer artefakt:

```bash
bash tools/install-linux-appdir.sh release-artifacts/Norscode-linux-<versjon>-AppDir.tar.gz
```

## Kva det gjer

Installerer under standard:

- `~/.local/opt/norscode-app/versions/<artefakt>`
- `~/.local/opt/norscode-app/current`
- `~/.local/opt/norscode-app/bin/norscode-app`

Dette gjer oppgradering enkel:

- ny versjon blir lagt i ny versjonskatalog
- `current` peikar til aktiv AppDir
- launcher peikar til `AppRun`

## Avgrensing

Dette er ein repeterbar brukarinstallasjon for AppDir-tarballen.

Det dekkjer ikkje enno:

- distro-spesifikk installasjon
- systemomfattande installasjon
- full AppImage-installasjon som primærveg
