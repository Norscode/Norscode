# Windows App Omganger

Denne planen beskriver kva som manglar for å få Norscode opp på same praktiske app-/distribusjonsnivå på Windows som vi no har på macOS og Linux.

Utgangspunktet er viktig:

- Windows har allereie brukarflate via `bin/nc.ps1` og `bin/nc.cmd`
- installasjonsskript finst i [tools/install.ps1](tools/install.ps1)
- men ein ekte Windows-native app-/release-linje er ikkje ferdig

Det betyr at Windows må delast i to:

1. først ein **native `.exe`-grunnmur**
2. deretter ein **Windows app-/installer-/release-linje**

## Omgang 1: Lås primærretning

Primærretninga for Windows bør vere:

1. ekte native `norscode.exe`
2. ZIP for enkel distribusjon
3. MSI som installasjons- og oppgraderingsformat når `.exe`-grunnmuren er stabil

Dette er den viktigaste forskjellen frå Linux og Mac:

- på Linux og macOS fanst native-sporet allereie tydeleg
- på Windows er native `.exe` framleis den første reelle terskelen

Ferdig når:

- planen seier eksplisitt at `native .exe` kjem før app-polish
- Windows-sporet prøver ikkje å hoppe direkte til installer/signering utan stabil `.exe`

## Omgang 2: Bygg første native Windows-artefakt

Bygg første verifiserbare Windows-native artefakt som kan publiserast som release-asset.

Målet er:

- `norscode.exe`
- ein enkel ZIP rundt binæren
- checksum

Ferdig når:

- repoet har ein dokumentert og repeterbar måte å produsere Windows-native artefakt på
- [tools/install.ps1](tools/install.ps1) kan peike på ein faktisk release-asset som finst

Status:
Bygd i første praktiske form, men framleis delvis. Repoet har no ein eksplisitt ZIP- og checksum-kontrakt via [tools/package-windows-app.no](tools/package-windows-app.no) og [docs/WINDOWS_APP_RELEASE.md](WINDOWS_APP_RELEASE.md), med same artefaktnamn som [tools/install.ps1](tools/install.ps1) forventar. Det som framleis manglar, er sjølve repeterbare `.exe`-produksjonen.

## Omgang 3: Windows launcher og app-layout

Når `.exe` er stabil, bygg ein enkel Windows app-layout rundt den.

Målet er:

- ryddig katalogstruktur for distribusjon
- launcher/alias der det gir meining
- første steg mot “app” i staden for berre enkeltfil-binær

Ferdig når:

- det finst ein enkel og dokumentert Windows app-layout
- ZIP-artefaktet inneheld meir enn berre laus `.exe` dersom det trengst

Status:
Bygd i første praktiske form. `tools/build-windows-app-layout.no` lagar no ein enkel `Norscode/`-struktur med `bin/norscode.exe`, `nc.exe`, `nc.cmd` og `nc.ps1`, og [tools/package-windows-app.no](tools/package-windows-app.no) pakkar denne layouten vidare i ZIP-artefaktet. Dette gjer Windows-sporet klarare som appflate sjølv før full installasjon/MSI.

## Omgang 4: Installasjon og oppgradering

Gjer Windows-installasjonen repeterbar og tydeleg for sluttbrukar.

Målet er:

- stabil installasjonssti
- oppgraderingshistorie
- klar skilnad mellom portable ZIP og installert variant

Aktuelle format:

- ZIP som første portable spor
- MSI som primær installasjonsveg når byggelinja er moden nok

Ferdig når:

- brukar kan installere og oppgradere utan manuell flytting av filer
- [tools/install.ps1](tools/install.ps1) samsvarer med faktisk release-struktur

Status:
Bygd i første praktiske form. [tools/install.ps1](tools/install.ps1) installerer no ZIP-layouten til ein versjonert struktur under `Prefix\versions\<versjon>\Norscode`, oppdaterer aktiv `Prefix\bin\`, og skriv aktiv versjon til `CURRENT_VERSION.txt`. Dette gir ei repeterbar installasjons- og oppgraderingsløype før MSI.

## Omgang 5: Windows CI og release

Kople Windows app-sporet inn i GitHub Actions og release-publisering.

Målet er:

- eigen workflow for Windows artefakt
- opplasting av ZIP og seinare MSI
- tydeleg release-kontrakt

Ferdig når:

- Windows-artefakt blir produsert automatisk i CI
- release-jobben publiserer det som [tools/install.ps1](tools/install.ps1) forventar

Status:
Bygd i første praktiske form, men framleis delvis. Repoet har no ein eigen workflow via [`.github/workflows/windows-app-release.yml`](../.github/workflows/windows-app-release.yml) og ei kort CI-skildring i [docs/WINDOWS_APP_CI.md](WINDOWS_APP_CI.md). Workflowen kan pakke og publisere ZIP-artefaktet automatisk, men stoppar enno eksplisitt dersom han ikkje finn ein faktisk `norscode.exe`. Den siste reelle blokkeringa for full grønn Windows-CI er derfor framleis repeterbar `.exe`-produksjon.

## Omgang 6: Signering og sluttstatus

Legg på Windows-trust og skriv ein kort sannheitsstatus for kva som er primær og sekundær distribusjon.

Målet er:

- code signing når sertifikat finst
- SmartScreen-venlegare distribusjon
- sluttrapport med `ferdig`, `delvis` og `manglar`

Ferdig når:

- Windows-sporet har ein eksplisitt primær distribusjonsveg
- signering er enten verifisert eller tydeleg markert som siste ytre blokkering

Status:
Bygd i første praktiske form. Repoet har no ein eksplisitt gap-status i [docs/WINDOWS_APP_GAP_STATUS.md](WINDOWS_APP_GAP_STATUS.md) og ei kort oppsummering i [docs/WINDOWS_APP_SLUTTRAPPORT.md](WINDOWS_APP_SLUTTRAPPORT.md). Primær distribusjonsveg er no tydeleg definert som ZIP + [tools/install.ps1](tools/install.ps1), medan MSI og code signing framleis er markerte som seinare produktfinish. Den viktigaste attverande blokkeringa er framleis første repeterbare `norscode.exe`.

## Anbefalt rekkefølgje

1. Native `.exe`
2. ZIP release
3. installasjon og oppgradering
4. CI/release
5. MSI
6. signering

## Kort vurdering

Windows ligg framleis bak Linux og macOS fordi:

- [docs/WINDOWS.md](WINDOWS.md) seier framleis at native `.exe` er planlagt
- [tools/install.ps1](tools/install.ps1) forventar ein Windows release-asset som ikkje er del av ein ferdig app-/release-linje enno
- det finst ikkje ein eigen Windows app-workflow på same nivå som:
  - [docs/MAC_APP_OMGANGER.md](MAC_APP_OMGANGER.md)
  - [docs/LINUX_APP_OMGANGER.md](LINUX_APP_OMGANGER.md)

Det gode er at retninga er ganske klar:

- først stabil Windows-native release
- deretter installer, signering og distribusjonsfinish
