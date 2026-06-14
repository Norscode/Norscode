# Mac-app gap-status

Dette er den korte sanninga om kor Mac-appsporet staar no.

## Ferdig

- lokal `.app`-bygging
- lokal installasjon til `~/Applications`
- lokal ad-hoc-signering
- lokal verifikasjon av bundle og codesign
- ZIP-release
- PKG-release
- eigen CI-workflow for macOS-app-artefakt
- første prosjektmal via `--config`
- GUI-variant med ekte macOS-vindauge som aktiv primær appretning

## Delvis

- djupare GUI-backend
  - GUI-hosten er no primær
  - men backend-en er framleis WebView-basert og enklast for rute-/GET-flyt

- Developer ID-signering i CI
  - workflowen er klar
  - manglar verifisert køyring med ekte secrets

- notarization
  - skript og CI-steg finst
  - manglar verifisert køyring med ekte Apple-credentials

- prosjektmal
  - fungerer for Norscode-baserte appar
  - er framleis tett knytt til dagens runtime-layout

## Manglar

- stabil DMG-linje
- universal binary
- full Gatekeeper-tillit bevist i praksis

## For aa lukke siste produksjonsgap

Sjå:

- [docs/MAC_APP_APPLE_SECRETS_CHECKLIST.md](/Users/jansteinar/Projects/Norscode1/docs/MAC_APP_APPLE_SECRETS_CHECKLIST.md)
- [docs/MAC_NATIVE_WINDOW_SLUTTRAPPORT.md](/Users/jansteinar/Projects/Norscode1/docs/MAC_NATIVE_WINDOW_SLUTTRAPPORT.md)

## Anbefalt primær distribusjon no

Bruk:

1. `PKG` for installasjon
2. `ZIP` som enkel fallback / manuell distribusjon

Ikkje gjer `DMG` til primær artefakt før han er stabil bevist.
