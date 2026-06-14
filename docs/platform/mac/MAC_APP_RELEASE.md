# Mac-app release

Dette dokumentet skildrar den første produktiserte release-linja for Norscode sin macOS-app.

## Kva repoet kan gjere no

Repoet kan no:

- bygge `.app` via `bash tools/build-macos-app.sh`
- installere lokalt via `bash tools/install-macos-app.sh`
- signere lokalt via `bash tools/sign-macos-app.sh`
- pakke release-artefakt via `bash tools/package-macos-app.sh`

## Standard release

Køyr:

```bash
bash tools/package-macos-app.sh
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
bash tools/package-macos-app.sh --format zip
```

Berre DMG:

```bash
bash tools/package-macos-app.sh --format dmg
```

Berre PKG:

```bash
bash tools/package-macos-app.sh --format pkg
```

Med eksplisitt app-path:

```bash
bash tools/package-macos-app.sh build/macos-app/Norscode.app
```

## Avgrensing

Denne omgongen dekkjer lokal artefaktpakking.

Det dekkjer ikkje enno:

- notarized release-artefakt
- universal binary
- stabil DMG-linje

Det høyrer til den vidare release-/CI-utvidinga.
