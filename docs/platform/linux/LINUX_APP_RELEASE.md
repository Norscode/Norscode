# Linux-app release

Dette dokumentet skildrar den første Linux app-/pakke-linja utover rein binær.

## Kva repoet kan gjere no

Repoet kan no:

- bygge Linux-native binær som før
- bygge ein `AppDir`-struktur
- pakke `AppDir` som `.tar.gz`
- prøve å bygge `AppImage` naar køyringa skjer paa Linux med `appimagetool`

## Standard køyring

```bash
./bin/nc run tools/package-linux-app.no
```

Det gjev:

- `build/linux-app/Norscode.AppDir`
- `release-artifacts/Norscode-linux-<versjon>-AppDir.tar.gz`
- checksum for tarball

Og i tillegg:

- `release-artifacts/Norscode-linux-<versjon>.AppImage` naar Linux + `appimagetool` finst

## Kvifor denne forma først

Sidan Linux-sporet allereie har sterk binær- og CI-grunnmur, er den første riktige appleveransen aa få på plass eit repeterbart `AppDir`.
Derfrå kan `AppImage` bli eit reint pakke-/publiseringslag, i staden for at all logikken blir skjult inni eitt format.

## Avgrensing

Denne omgongen beviser:

- Linux app-layout
- tarball-artefakt
- AppImage-best-effort

Det beviser ikkje enno:

- full AppImage-verifikasjon
- distro-spesifikk pakking
- signering/attestering
