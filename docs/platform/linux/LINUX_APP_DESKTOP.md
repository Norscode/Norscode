# Linux-app desktop-integrasjon

Dette dokumentet skildrar desktop-integrasjonen for Linux app-sporet.

## Kva repoet kan gjere no

Repoet kan no:

- bygge `AppDir` via `bash tools/package-linux-app.sh --format appdir`
- installere `.desktop`-entry lokalt via `bash tools/install-linux-desktop-entry.sh`
- installere ikon under brukaren si lokale `~/.local/share`-flate

## Standard flyt

```bash
bash tools/package-linux-app.sh --format appdir
bash tools/install-linux-desktop-entry.sh
```

Dette gjev:

- lokal `.desktop`-entry
- lokal ikonregistrering
- `Exec=` peikar mot `AppRun` i den bygde `AppDir`

## Avgrensing

Dette er bruker-lokal desktop-integrasjon, ikkje systemvid installasjon.

Det dekkjer ikkje enno:

- mime/file associations
- systemomfattande desktop-installasjon
- pakkeformatspesifikk desktop-registrering for `deb`/`rpm`
