# Windows App Release

Dette dokumentet skildrar den første praktiske release-kontrakten for Windows-sporet.

## Sanning først

Windows har enno ikkje ein full, bevist native `.exe`-byggekøyring i denne repo-omgangen.

Det som no er bygd, er derfor:

- ein **pakke- og release-kontrakt**
- ein **enkel app-layout**
- ikkje enno ein full `.exe`-generator

Det betyr at Omgang 2 er lukka i første praktiske form, men framleis delvis:

- når ein gyldig `norscode.exe` finst
- kan repoet no pakke han til riktig release-format
- med same naming som Windows-installasjonsskriptet forventar

## Release-kontrakt

Skript:

- [tools/package-windows-app.no](../tools/package-windows-app.no)

Bruk:

```bash
NORSCODE_WINDOWS_PACKAGE_EXE=path/to/norscode.exe ./bin/nc run tools/package-windows-app.no
```

Output:

- `release-artifacts/norscode-windows-<versjon>.zip`
- `release-artifacts/norscode-windows-<versjon>.zip.sha256`

ZIP-en inneheld:

- `Norscode/bin/norscode.exe`
- `Norscode/bin/nc.exe`
- `Norscode/bin/nc.cmd`
- `Norscode/bin/nc.ps1`
- `Norscode/README.txt`

## Kvifor dette er viktig

[tools/install.ps1](../tools/install.ps1) ser etter ein release-asset som liknar:

- `norscode-windows*`

Denne release-kontrakten gjer no det eksplisitt kva artefaktformat Windows-sporet skal publisere når ein ekte `.exe` finst.

## Status

Ferdig:

- ZIP-kontrakt
- checksum-kontrakt
- artefaktnamn som samsvarer med installasjonsløypa
- enkel Windows app-layout

Delvis:

- sjølve `.exe`-produksjonen
- automatisk CI er no klargjord, men er framleis blokkert av `.exe`-produksjonen

Manglar:

- repeterbar native Windows `.exe`-bygging
- MSI
- code signing

Den første CI-linja for dette sporet er no dokumentert i [docs/WINDOWS_APP_CI.md](WINDOWS_APP_CI.md).

## Neste naturlege steg

1. få fram første faktiske `norscode.exe`
2. kople `tools/package-windows-app.no` til den
3. legg ZIP-artefaktet inn i GitHub Actions og release
