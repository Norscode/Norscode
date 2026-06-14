# Mac Native Window Sluttrapport

Dette er kort sluttrapport for sporet som gjer Norscode-appen paa macOS til ei **ekte vindusapp**, ikkje berre ein Terminal-launcher.

## Sluttstatus

Ferdig i første praktiske form:

- ekte `NSWindow`-host via AppKit
- WebView-basert backend-retning som primær første host
- Norscode-driven HTML-rendering inn i vindauget
- enkel intern navigasjon
- enkel rute-state (`path` + `query`)
- reload via appmeny og `Cmd-R`
- GUI-varianten kopla inn i den vanlege `.app`-/pakke-/CI-linja

## Det viktigaste skiftet

Før:

- Mac app-sporet var i praksis ein godt pakka Terminal-wrapper

No:

- Mac app-sporet har ein faktisk GUI-variant som primærretning
- Terminal-varianten finst framleis, men er sekundær vedlikehaldsflate

## Kva som er primær Mac-oppleving no

Primær:

1. GUI-appen bygd via [tools/build-macos-app.sh](tools/build-macos-app.sh)
2. GUI-pakking via [tools/package-macos-app.sh](tools/package-macos-app.sh)
3. GUI-bygg i [`.github/workflows/macos-app-release.yml`](../.github/workflows/macos-app-release.yml)

Sekundær:

1. `bash tools/build-macos-app.sh --mode terminal`

## Kva som framleis er delvis

- backend-en er framleis WebView-basert
- inputsløyfa er framleis enklast for GET/rute-navigasjon
- full Apple-trust krev framleis Developer ID + notarization i ekte CI

## Kva som manglar om målet er endaa djupare native app

- rikare tovegsbro mellom GUI-host og Norscode
- meir komplett skjema-/submit-modell
- eventuelt seinare native UI/AppKit-backend utan WebView

## Konklusjon

Norscode paa macOS er no ikkje berre i stand til aa levere ein `.app`.
Det kan no levere ein **GUI-app med ekte vindauge** som den primære appretninga.

Det som staar igjen vidare er produktmodning og djupare UI-runtime, ikkje lenger den grunnleggjande overgangen fraa Terminal-app til ekte vindusapp.
