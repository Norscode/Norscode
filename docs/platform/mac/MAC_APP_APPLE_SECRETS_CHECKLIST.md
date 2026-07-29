# Mac-app Apple secrets checklist

Dette dokumentet er den korte, operative lista for aa gjere macOS-appsporet heilt produksjonsklart i GitHub Actions.

## Naudsynte GitHub secrets

Legg desse inn i repo-secrets:

1. `MACOS_CODESIGN_IDENTITY`
2. `APPLE_ID`
3. `APPLE_TEAM_ID`
4. `APPLE_APP_PASSWORD`

## Kva dei betyr

- `MACOS_CODESIGN_IDENTITY`
  - full codesign-identitet, til dømes:
  - `Developer ID Application: Namn Etternamn (TEAMID)`

- `APPLE_ID`
  - Apple-ID som skal brukast med `notarytool`

- `APPLE_TEAM_ID`
  - Apple Developer team-id

- `APPLE_APP_PASSWORD`
  - app-spesifikt passord brukt av `notarytool`

## Før du legg inn secrets

Bekreft lokalt:

1. `security find-identity -v -p codesigning`
2. `./bin/nc run tools/build-macos-app.no`
3. `NORSCODE_MACOS_SIGN_IDENTITY="Developer ID Application: ..." NORSCODE_MACOS_SIGN_VERIFY=1 ./bin/nc run tools/sign-macos-app.no`
4. `NORSCODE_MACOS_PACKAGE_FORMAT=pkg ./bin/nc run tools/package-macos-app.no`

## Når secrets er lagt inn

Køyr workflowen:

- [`.github/workflows/macos-app-release.yml`](/Users/jansteinar/Projects/Norscode1/.github/workflows/macos-app-release.yml)

Start med:

1. `workflow_dispatch`
2. deretter tag-bygd release når første manuelle køyring er verifisert

## Kva som skal bli grønt i første ekte Apple-køyring

1. `Signer macOS app (Developer ID)`
2. `Notariser PKG`
3. artefaktopplasting med:
   - `.pkg`
   - `.pkg.sha256`
   - `.zip`
   - `.zip.sha256`

## Kva som framleis kan vere gult utan aa blokkere

- DMG manglar eller feilar
- universal binary manglar

Det skal ikkje blokkere første produksjonsklare Mac-release, saa lenge:

- PKG er signert
- PKG er notarisert
- ZIP finst som fallback

## Definisjon paa heilt grønt

Mac-sporet kan kallast heilt grønt naar:

1. PKG er signert med Developer ID i CI
2. PKG er notarisert i CI
3. GitHub Release publiserer PKG og ZIP automatisk
4. installasjon fraa PKG er verifisert paa rein Mac

DMG og universal binary er deretter kvalitet-/komfortforbetringar, ikkje lenger hovudblokkeringar.
