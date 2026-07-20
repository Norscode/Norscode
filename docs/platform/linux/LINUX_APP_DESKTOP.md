# Linux-app desktop-integrasjon

Dette dokumentet skildrar desktop-integrasjonen for Linux app-sporet.

## Kva repoet kan gjere no

Repoet kan no:

- bygge `AppDir` via `NORSCODE_LINUX_PACKAGE_FORMAT=appdir ./bin/nc run tools/package-linux-app.no`
- installere `.desktop`-entry lokalt via `./bin/nc run tools/install-linux-desktop-entry.no`
- installere ikon under brukaren si lokale `~/.local/share`-flate

## Standard flyt

```bash
NORSCODE_LINUX_PACKAGE_FORMAT=appdir ./bin/nc run tools/package-linux-app.no
./bin/nc run tools/install-linux-desktop-entry.no
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
