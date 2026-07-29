# Mac-app release

Dette dokumentet skildrar den første produktiserte release-linja for Norscode sin macOS-app.

## Kva repoet kan gjere no

Repoet kan no:

- bygge `.app` via `./bin/nc run tools/build-macos-app.no`
- installere lokalt via `./bin/nc run tools/install-macos-app.no`
- signere lokalt via `bash tools/sign-macos-app.no`
- pakke release-artefakt via `./bin/nc run tools/package-macos-app.no`

## Standard release

Køyr:

```bash
./bin/nc run tools/package-macos-app.no
```

Det lagar som standard primært:

- `release-artifacts/Norscode-macos-<versjon>.zip`
- `release-artifacts/Norscode-macos-<versjon>.pkg`
- tilhøyrande `.sha256`-filer

I tillegg prøver det aa lage:

- `release-artifacts/Norscode-macos-<versjon>.dmg`

Viss `hdiutil` feilar lokalt, blir ZIP-artefakten framleis produsert og kan brukast som fallback.
PKG-løypa gir i tillegg eit installasjonsartefakt som ikkje er avhengig av DMG-bana.

## Variantar

Berre ZIP:

```bash
NORSCODE_MACOS_PACKAGE_FORMAT=zip ./bin/nc run tools/package-macos-app.no
```

Berre DMG:

```bash
NORSCODE_MACOS_PACKAGE_FORMAT=dmg ./bin/nc run tools/package-macos-app.no
```

Berre PKG:

```bash
NORSCODE_MACOS_PACKAGE_FORMAT=pkg ./bin/nc run tools/package-macos-app.no
```

Med eksplisitt app-path:

```bash
NORSCODE_MACOS_PACKAGE_APP=build/macos-app/Norscode.app ./bin/nc run tools/package-macos-app.no
```

## Avgrensing

Denne omgongen dekkjer lokal artefaktpakking.

Det dekkjer ikkje enno:

- notarized release-artefakt
- universal binary
- stabil DMG-linje

Det høyrer til den vidare release-/CI-utvidinga.
