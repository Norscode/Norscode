# Mac-app i omganger

Dette dokumentet deler arbeidet med aa gjere Norscode i stand til aa lage og levere Mac-appflater inn i små, verifiserbare omganger.

Maalet er ikkje berre aa kunne lage ein Mach-O-binær, men aa kunne levere ein faktisk macOS-app som brukarar kan opne, installere og stole paa.

## Sluttdefinisjon

Mac-appsporet er ferdig naar:

- Norscode kan pakke ein fungerande `.app` rundt den native runtime-en
- appen kan opnast lokalt paa macOS utan manuell repo-kunnskap
- appen har stabil bundle-struktur, ikon og `Info.plist`
- release-linja kan produsere signerte artefakt
- notarization og distribusjon er mekanisk verifiserte

## Omgang 1

Maal:
Faa paa plass ein minimal, usignert `.app`-bundle for lokal utvikling og Finder-verifikasjon.

Leveransar:

- `tools/build-macos-app.sh`
- ekte `.app` under `build/macos-app/`
- `Info.plist`, ikon og launcher
- bundla `bin/nc` + `dist/norscode_native` + naudsynte runtime-filer

Ferdig naar:

- `bash tools/build-macos-app.sh` lagar `build/macos-app/Norscode.app`
- `Contents/Info.plist` er gyldig
- app-bundlen inneheld lokal runtime og kan opne Terminal med den bundla CLI-en

Status:
Bygd i repoet. `tools/build-macos-app.sh` lagar no ein usignert `Norscode.app` med launcher, ikon, `Info.plist` og bundla runtime. Neste naturlege forbetring er aa stramme inn payload og betre launcher-opplevinga.

## Omgang 2

Maal:
Forbetre app-opplevinga slik at bundlen kjennest som ein faktisk utviklarapp, ikkje berre ein Finder-wrapper.

Leveransar:

- betre launcher-UX for dobbeltklikk
- eigen velkomst-/kommandoflate
- dokumentopning for relevante filtypar
- avklarte bundle-identitetar og versjonsfelt

Ferdig naar:

- dobbelklikk gjev ei tydeleg og nyttig førsteoppleving
- `.no`/Norscode-filer kan knytast mot app-bundlen utan manuelle hack

Status:
Bygd i repoet. App-bundlen har no betre Terminal-velkomst, opnar Finder-innsendte `.no`-filer via `./bin/nc run`, og eksporterer dokumenttypar i same `Info.plist` som sjølve appen.

## Omgang 3

Maal:
Gjer app-bundlen installasjonsklar paa lokal maskin.

Leveransar:

- install- og oppdateringsskript for `.app`
- standard plassering under `~/Applications` eller `/Applications`
- release-layout som skil CLI-arkiv og Mac-app-artefakt

Ferdig naar:

- lokal installasjon ikkje krev repoet etter bygg
- ein ny brukar kan faa appen paa plass utan manuell strukturkunnskap

Status:
Bygd i repoet. `tools/install-macos-app.sh` installerer no den bygde appen til `~/Applications` som standard, med versjonert lagring under `~/Applications/.Norscode/versions/` og ein stabil aktiv lenke for oppdatering.

## Omgang 4

Maal:
Legg paa Apple-krava for tillit og distribusjon.

Leveransar:

- `codesign`-steg
- entitlements der det trengst
- notarization-steg
- stapling/verifikasjon

Ferdig naar:

- appen opnar utan unødig Gatekeeper-friksjon
- signering og notarization er dokumenterte og repeterbare

Status:
Delvis bygd i repoet. Lokal codesign og verifikasjon er no paa plass via `tools/sign-macos-app.sh` og `tools/verify-macos-app.sh`, og CI-linja er klargjord for Developer ID og notarization naar secrets finst. Full Gatekeeper-tillit er framleis ikkje bevist i praksis, fordi det krev ekte Developer ID, Apple-credentials og release-køyring.

## Omgang 5

Maal:
Produktiser Mac-release-linja.

Leveransar:

- `.dmg` eller `.pkg`
- checksums og release-artefakt
- CI-jobb for macOS-bundling
- eventuelt universal-binary-spor

Ferdig naar:

- GitHub Release kan produsere ein Mac-app-artefakt paa same maate kvar gong

Status:
Delvis bygd i repoet. `tools/package-macos-app.sh` produserer no ZIP- og PKG-artefakt med checksums lokalt, og desse er den reelle primærflata. Ein eigen CI-workflow byggjer og lastar no opp macOS-app-artefakt, medan stabil DMG-linje, notarized release og eventuell universal-binary framleis ligg att.

## Omgang 6

Maal:
Gjer Norscode i stand til aa vere praktisk app-byggar for andre prosjekt paa macOS.

Leveransar:

- app-mal eller scaffold for Norscode-prosjekt
- stabil kontrakt for ressursar, bundle-id og entrypoint
- dokumentasjon for korleis brukarar lagar eigne Mac-appar

Ferdig naar:

- ein brukar kan bygge sin eigen Norscode-baserte Mac-app utan aa kjenne interne repo-detaljar

Status:
Bygd i første praktiske form. `tools/build-macos-app.sh` støttar no `--config`, repoet har ein mal i `tools/macos-app.template.env`, og prosjektstien er dokumentert i `docs/MAC_APP_TEMPLATE.md`.

Oppdatert status:
GUI-varianten er no den aktive primærretninga for Mac-appen. Terminal-launcheren finst framleis, men er ikkje lenger den sanne hovudopplevinga.

## Prioritet no

1. Omgang 1: minimal lokal `.app`-bundle
2. Omgang 2: betre launcher og filtype-opning
3. Omgang 3: installasjon og artefaktstruktur
4. Omgang 4: signering og notarization
5. Omgang 5: release- og CI-produksjon
6. Omgang 6: prosjektmal for eigne appar
