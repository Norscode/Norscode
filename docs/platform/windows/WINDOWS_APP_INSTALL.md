# Windows App Install

Dette dokumentet skildrar den første repeterbare installasjons- og oppgraderingsløypa for Windows-sporet.

## Mål

Gi Windows ei stabil installasjonsflate rundt den versjonerte app-layouten, utan å vente på MSI.

## Modell

Installasjonen brukar no:

- `Prefix\versions\<versjon>\Norscode\...`
- `Prefix\bin\...` som aktiv brukarflate
- `Prefix\CURRENT_VERSION.txt` som enkel aktiv versjonsmarkør

Det betyr:

- kvar installasjon kan leve versjonert
- den aktive `bin`-mappa blir oppdatert til siste versjon
- oppgradering krev ikkje manuell flytting av filer

## Installer

Skript:

- [tools/install.ps1](../tools/install.ps1)

Skriptet:

1. lastar ned `norscode-windows-<versjon>.zip`
2. pakkar ut til ein versjonert katalog
3. oppdaterer `bin\`
4. skriv aktiv versjon til `CURRENT_VERSION.txt`
5. legg `bin\` til `PATH` dersom nødvendig

## Skilnad frå portable ZIP

Portable ZIP:

- kan pakkast ut kvar som helst
- er fin for manuell testing

Installert variant:

- bruker stabil `Prefix`
- har versjonert lagring
- oppdaterer aktiv `bin`-flate

## Status

Ferdig i første praktiske form:

- versjonert layout
- aktiv `bin`-flate
- repeterbar oppgraderingsmodell

Manglar framleis:

- MSI
- code signing
- CI/release for Windows
