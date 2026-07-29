# Windows-app sluttrapport

Denne rapporten er den korte sanninga om kva som no gjeld for Windows-appsporet.

## Konklusjon

Windows-sporet er no bygd til eit første praktisk app-/distribusjonsnivå rundt ein kommande native `norscode.exe`:

- Windows har ein eksplisitt ZIP-release-kontrakt
- Windows har ein enkel app-layout med `bin/`-flate
- Windows har repeterbar installasjon og oppgradering
- Windows har ein eigen CI-/release-workflow for ZIP-artefakt

## Primær veg

Den eine klare, dokumenterte Windows-vegen no er:

1. produser `norscode.exe`
2. pakk `norscode-windows-<versjon>.zip`
3. installer via [tools/install.ps1](../tools/install.ps1)

## Sekundær veg

Det finst ingen sann sekundær appveg paa Windows enno som er paa same modningsnivå som Mac GUI eller Linux AppDir.

MSI er planlagt som seinare installasjonsveg, men er framleis sekundær framtidsretning, ikkje aktiv sannhet.

## Praktisk tolkning

Windows-sporet har no mykje av distribusjonsramma paa plass.
Det som framleis blokkerer “heilt grønt” Windows-spor, er ikkje fleire docs eller ZIP-detaljar, men den første repeterbare native `.exe`-produksjonen.

Etter at den er paa plass, blir resten naturleg:

- Windows CI kan gå grønt ende til ende
- ZIP-installasjonen blir fullt bevist
- MSI og signering kan byggjast som produktfinish
