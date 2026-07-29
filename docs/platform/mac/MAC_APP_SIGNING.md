# Mac-app signering

Dette dokumentet skildrar den praktiske signeringsstigen for Norscode sin macOS-app.

## Kva er paa plass

Repoet har no:

- bygging av lokal `.app` via `./bin/nc run tools/build-macos-app.no`
- installasjon via `./bin/nc run tools/install-macos-app.no`
- signering via `bash tools/sign-macos-app.no`
- verifikasjon via `./bin/nc run tools/verify-macos-app.no`

## Lokal utvikling

For lokal testing utan Apple-utviklaridentitet kan du bruke ad-hoc-signering:

```bash
./bin/nc run tools/build-macos-app.no
NORSCODE_MACOS_SIGN_VERIFY=1 ./bin/nc run tools/sign-macos-app.no
```

Dette gjev ikkje notarization, men det gjev ein konsistent lokal codesign-status og ein mekanisk verifikasjon av app-bundlen.
Gatekeeper (`spctl`) kan framleis avvise appen i denne modusen; det er forventa før Developer ID-signering og notarization.

## Developer ID

Når du har ein lokal Developer ID Application-identitet, kan du signere slik:

```bash
NORSCODE_MACOS_SIGN_IDENTITY="Developer ID Application: Ditt Namn (TEAMID)" NORSCODE_MACOS_SIGN_VERIFY=1 ./bin/nc run tools/sign-macos-app.no
```

Du kan liste identitetar med:

```bash
security find-identity -v -p codesigning
```

## Notarization

Repoet har no eit eige notarization-skript:

```bash
NORSCODE_MACOS_NOTARIZE_ARTIFACT=release-artifacts/Norscode-macos-<versjon>.pkg ./bin/nc run tools/notarize-macos-app.no
```

Det krev desse miljøvariablane:

- `APPLE_ID`
- `APPLE_TEAM_ID`
- `APPLE_APP_PASSWORD`

Dette er neste naturlege steg etter signering:

1. bygg `.app`
2. signer med Developer ID
3. pakk i `.zip` eller `.dmg`
4. notariser med `notarytool`
5. staple notarization-resultatet
6. verifiser med `spctl`

## Avgrensing

Omgang 4 i repoet dekkjer:

- lokal codesign-løype
- verifikasjon
- dokumentert overgang til notarization

Det dekkjer ikkje enno:

- full Gatekeeper-godkjenning
- notarization verifisert med ekte credentials i denne repo-konteksten
- stapling for alle artefakttypar i releaseflyt
