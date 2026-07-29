# Mac-app mal

Dette dokumentet skildrar den enkle app-kontrakten for aa bruke Norscode sin macOS-app-linje i andre prosjekt.

## Minstekontrakt

Ein prosjektmal treng berre:

- ein `dist/norscode_native`
- ein `bin/nc`
- naudsynte runtime-mapper som builderen allereie bundlar
- miljøvariabler for app-identitet

## Miljøvariabler

Du kan bruke [tools/macos-app.template.env](/Users/jansteinar/Projects/Norscode1/tools/macos-app.template.env) som eksempel på verdier.

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
NORSCODE_MACOS_APP_NAME="MittProdukt" \
NORSCODE_MACOS_APP_BUNDLE_ID="com.example.mittprodukt" \
NORSCODE_MACOS_APP_VERSION="1.2.3" \
NORSCODE_MACOS_APP_ICON="frontend/assets/icons/norscode.icns" \
NORSCODE_MACOS_WELCOME_TITLE="MittProdukt på macOS" \
NORSCODE_MACOS_WELCOME_MESSAGE="Velkomen til MittProdukt." \
./bin/nc run tools/build-macos-app.no
```

Valfrie miljøvariabler:

- `NORSCODE_MACOS_APP_NAME`
- `NORSCODE_MACOS_APP_BUNDLE_ID`
- `NORSCODE_MACOS_APP_VERSION`
- `NORSCODE_MACOS_APP_OUT`

## Pakk og installer

```bash
NORSCODE_MACOS_INSTALL_APP=build/macos-app/MittProdukt.app NORSCODE_MACOS_INSTALL_NAME=MittProdukt.app ./bin/nc run tools/install-macos-app.no
NORSCODE_MACOS_PACKAGE_FORMAT=pkg NORSCODE_MACOS_PACKAGE_APP=build/macos-app/MittProdukt.app ./bin/nc run tools/package-macos-app.no
```

## Kva som framleis er repo-spesifikt

Denne malen gjenbruker framleis den eksisterande Norscode-runtime-strukturen og CLI-løypa.
Han er derfor ein praktisk prosjektmal for Norscode-baserte appar, ikkje enno ein full generisk app-byggar for kva som helst.
