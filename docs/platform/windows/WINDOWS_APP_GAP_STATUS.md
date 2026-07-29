# Windows-app gap-status

Dette er den korte sanninga om kor Windows-appsporet staar no.

## Ferdig

- ZIP-release-kontrakt
- checksum for Windows ZIP-artefakt
- enkel Windows app-layout rundt `norscode.exe`
- repeterbar installasjons- og oppgraderingsløype rundt ZIP-layout
- eigen CI-/release-workflow for Windows app-artefakt

## Delvis

- Windows-native `.exe`
  - release-, layout- og CI-kontrakten er no bygd rundt han
  - men sjølve repeterbare `.exe`-produksjonen er framleis ikkje bevist ende til ende

- Windows app-CI
  - workflowen finst og kan pakke/publisere ZIP
  - men han stoppar enno eksplisitt dersom `norscode.exe` ikkje finst

## Manglar

- stabilt verifisert `norscode.exe` bygd automatisk i repoet
- MSI
- code signing
- SmartScreen-/trust-finish

## Anbefalt primær distribusjon no

Bruk:

1. `norscode-windows-<versjon>.zip`
2. installasjon via [tools/install.ps1](../tools/install.ps1)

Ikkje gjer MSI eller signering til “mentalt ferdig” før den første ekte `.exe`-kjeda er på plass.
