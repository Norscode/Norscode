# Mac-app mal

Dette dokumentet skildrar den enkle app-kontrakten for aa bruke Norscode sin macOS-app-linje i andre prosjekt.

## Minstekontrakt

Ein prosjektmal treng berre:

- ein `dist/norscode_native`
- ein `bin/nc`
- naudsynte runtime-mapper som builderen allereie bundlar
- ein konfigurasjonsfil for app-identitet

## Konfigfil

Du kan bruke:

- [tools/macos-app.template.env](/Users/jansteinar/Projects/Norscode1/tools/macos-app.template.env)

Eksempel:

```sh
APP_NAME="MittProdukt"
BUNDLE_ID="com.example.mittprodukt"
APP_VERSION="1.2.3"
ICON_PATH="frontend/assets/icons/norscode.icns"
DOC_TYPE_SOURCE_NAME="MittProdukt Source"
DOC_TYPE_UTILITY_NAME="MittProdukt Utility Source"
DOC_TYPE_BYTECODE_NAME="MittProdukt Bytecode JSON"
WELCOME_TITLE="MittProdukt på macOS"
WELCOME_MESSAGE="Velkomen til MittProdukt."
```

## Bygg

```bash
bash tools/build-macos-app.sh --config path/to/app.env
```

Valfrie override-flag:

- `--name`
- `--bundle-id`
- `--version`
- `--out`

## Pakk og installer

```bash
bash tools/install-macos-app.sh build/macos-app/MittProdukt.app --name MittProdukt
bash tools/package-macos-app.sh --format pkg build/macos-app/MittProdukt.app
```

## Kva som framleis er repo-spesifikt

Denne malen gjenbruker framleis den eksisterande Norscode-runtime-strukturen og CLI-løypa.
Han er derfor ein praktisk prosjektmal for Norscode-baserte appar, ikkje enno ein full generisk app-byggar for kva som helst.
