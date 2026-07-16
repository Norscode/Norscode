# V7400 Active bundle path trace

## Formål
Dette steget skil mellom:
- den emulerte patched single-module-logikken
- det faktiske `bygg_bundle(...)`-kallet i aktiv bane

## Probe 1: emulert branch
`tests/fixtures/v7400/bundler_single_module_emulated_v7400.no`

Denne køyrer same hovudsteg som den patched single-module-grena:
- `les_ncb_eller_kompiler(...)`
- `fil_skriv(...)`
- lengde/skriv

## Probe 2: direkte `bygg_bundle(...)`
`tests/fixtures/v7400/bundler_single_module_direct_v7400.no`

Denne kallar `bygg_bundle(...)` direkte med same single-module-arg.

## Tolkning
- dersom emulert branch er grøn og direkte branch er raud, sit feilen inne i `bygg_bundle(...)`-sporet snarare enn i hjelpefunksjonen
- dersom begge er raude, sit feilen under hjelpefunksjonen eller i aktiv compile-path
