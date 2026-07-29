# Linux-app i omganger

Dette dokumentet deler Linux-appsporet inn i små, verifiserbare omganger.

Maalet er ikkje berre aa ha ein Linux-binær, men aa ha ein distribuerbar Linux-appflate med installasjon, desktop-integrasjon og repeterbar release.

## Sluttdefinisjon

Linux-appsporet er ferdig naar:

- Norscode kan levere ein stabil Linux-appflate utover rein binær
- installasjon, oppgradering og drift er repeterbar
- minst eitt primært Linux-pakkeformat er støtta og verifisert
- desktop-integrasjon er paa plass der det er relevant
- CI/release kan produsere Linux-app-artefakt automatisk

## Kva som allereie finst

Repoet har allereie mykje av fundamentet:

- Linux native binary og CI er paa plass
- ELF/selfcompile-paritet er hardt verifisert i CI
- generell Linux-installasjon finst i `tools/install`
- driftsspor finst i `docs/SYSTEMD.md` og `deploy/norscode.service`

Det som manglar er derfor mest app-/pakke-/desktop-laget.

## Omgang 1

Maal:
Definer éin aktiv Linux-appretning og dokumenter kva som er primær distribusjon.

Leveransar:

- kort Linux-app-plan
- avklart primær artefaktretning
- avklart skilje mellom CLI-binær og Linux-app-/pakke-løype

Ferdig naar:

- det er tydeleg om Linux primært skal leverast som `AppImage`, `deb`, `rpm` eller anna

## Omgang 2

Maal:
Lag første produktiserte Linux-app-artefakt.

Leveransar:

- eige Linux pakkeskript
- minst eitt fungerande artefaktformat
- checksums

Ferdig naar:

- repoet kan bygge eit Linux app-/pakke-artefakt lokalt utan manuell spesialkunnskap

Status:
Bygd i første praktiske form. `tools/package-linux-app.no` lagar no eit `AppDir`, pakkar det som tarball, og prøver `AppImage` som beste innsats naar køyringa skjer paa Linux med `appimagetool`.

## Omgang 3

Maal:
Legg paa Linux desktop-integrasjon der det gir meining.

Leveransar:

- `.desktop`-fil
- ikonplassering
- eventuelle mime/file associations

Ferdig naar:

- appen kan integrerast i vanleg Linux desktop-oppleving, ikkje berre køyrast fraa shell

Status:
Bygd i første praktiske form. `tools/install-linux-desktop-entry.no` installerer no `.desktop`-entry og ikon fraa `AppDir` til brukarens lokale desktop-flate, og flyten er dokumentert i `docs/LINUX_APP_DESKTOP.md`.

## Omgang 4

Maal:
Gjer Linux installasjon og oppgradering repeterbar.

Leveransar:

- install-/oppgraderingsskript for valt artefaktformat
- dokumentasjon for brukarinstallasjon
- eventuell systemintegrasjon for service-mode

Ferdig naar:

- ein Linux-brukar kan installere utan repo-kunnskap

Status:
Bygd i første praktiske form. `tools/install-linux-appdir.no` installerer no AppDir-tarballen repeterbart med versjonert layout og aktiv `current`-lenke, og brukarinstallasjonen er dokumentert i `docs/LINUX_APP_INSTALL.md`.

## Omgang 5

Maal:
Produktiser Linux-release og CI.

Leveransar:

- eigen Linux app-/pakke-workflow
- artefaktopplasting i release
- checksums og enkel verifikasjon

Ferdig naar:

- GitHub Release kan produsere Linux app-/pakke-artefakt mekanisk

Status:
Bygd i første praktiske form. Ein eigen workflow i `.github/workflows/linux-app-release.yml` byggjer no Linux app-artefakt, lastar opp AppDir-tarball og prøver AppImage som beste innsats for GitHub Release.

## Omgang 6

Maal:
Stram inn kvalitet og portabilitet.

Leveransar:

- eventuell fleire pakkeformat
- eventuell `linux-arm64`
- eventuell signering/attestering
- sluttstatus med kva som er primær og kva som er tillegg

Ferdig naar:

- Linux-sporet har éin sann primær distribusjonsveg og eventuelle tillegg er eksplisitt sekundære

Status:
Lukka i første praktiske form. Linux-sporet har no éin eksplisitt primær veg via `AppDir` og `AppDir.tar.gz`, medan `AppImage` er tona ned til sekundær beste innsats inntil han er bevist grønt i praksis.

## Anbefalt rekkjefølgje

1. Primær Linux-retning: `PKG`-tankegang finst ikkje her, saa vel éin av:
   - `AppImage` for enkel distribusjon
   - `deb` for Debian/Ubuntu-fokus
2. Bygg første artefakt og checksums
3. Legg paa `.desktop` + ikon
4. Legg artefaktet inn i CI/release
5. Utvid berre etter at primærveg er stabil

## Mi vurdering no

Linux er nærmare mål enn Windows, fordi:

- native runtime og verifikasjon allereie er sterke
- installasjon og service-mode allereie finst
- den største mangelen er ikkje kompilatorkjernen, men pakke- og desktoplaget
