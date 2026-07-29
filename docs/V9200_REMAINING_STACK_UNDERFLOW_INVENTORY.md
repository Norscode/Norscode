# V9200 Remaining stack-underflow inventory

## Formål
Dette steget samlar dei attverande kjende mønstra som framleis kan blokkere AI-sjølvutvikling, særleg i aktiv compile/bundle-bane.

## Verifisert blokkering no
Aktiv bane er framleis raud for:
- direkte `run selfhost/bundler.no` med `NORSCODE_BUNDLE_ARGS`
- full bootstrap refresh via `NORSCODE_REGEN_BOOTSTRAP_FULL=1 ./bin/nc run tools/nc_regen_bootstrap.no`

## Kjent grøn kontrast
Dette verkar:
- emulert helper-bane
- split-bygd liste sendt til `bygg_bundle(...)`
- plain compile av `selfhost/lexer/lexer_m1.no`

## Høg prioritet: ekte aktive kjelder

### 1. `selfhost/nc_main.no`
Framleis attverande listebygging i aktiv bane:
- `la alle_rh: liste = []`
- `la alle_rh_merged: liste = []`
- `la imports: liste<tekst> = []`
- `la hook_args: liste = []`
- `la trace = []`
- `la modular = []` i `nc_l5b_gen2`

Vurdering:
- `nc_main.no` er framleis den viktigaste fila etter `bundler.no`
- særleg alle stader der tom liste blir bygd og så mutert med `legg_til(...)`

### 2. `selfhost/bundler.no`
Status:
- `bundler.start()` er herda
- single-module branch er herda

Men:
- aktiv direkte bane er framleis raud, så meir indirekte påverknad frå `nc_main` eller underliggande compile-path er framleis sannsynleg

### 3. `selfhost/common.no`
Funne mange tom-liste-initialiseringar i compile/assembler-liknande kode:
- `stack: liste_heltall = []`
- `ops: liste_tekst = []`
- `verdier: liste_heltall = []`
- `tokens: liste_tekst = []`
- `asm: liste_tekst = []`

Vurdering:
- denne fila er ein sterk kandidat for djupare compile/codegen-feil
- særleg dersom aktiv bane framleis brukar eldre/common-spor internt

### 4. `selfhost/vm.no`
Funne mange tom-liste-initialiseringar og `legg_til(...)`

Vurdering:
- dette er tung og sentral runtime-kjelde
- ikkje første patchmål for bundle-inngangen, men høg risiko dersom compile-path framleis toler listebygging dårleg generelt

## Mellom prioritet

### `selfhost/lsp/server.no`
Mange tom-liste-initialiseringar att:
- `responses = []`
- `notifications = []`
- `methods = []`
- `trace = []`
- `chunks = []`

Vurdering:
- viktig for brei robustheit
- mindre direkte relevant for dagens bootstrap-refresh-blokk enn `nc_main`, `common`, `vm`

### `selfhost/elf_compile_driver.no`
- `la reine_modular: liste<tekst> = []`

Vurdering:
- relevant for stage0 / ELF-compile-sporet
- bør herdast etter `nc_main`

### `selfhost/selfcompile_l5.no`
### `selfhost/selfcompile_l5b.no`
- `la m: liste<tekst> = []`

Vurdering:
- relevante for selfcompile-spor
- mellomprioritet

## Lågare prioritet / ikkje primær blokkering no
- `selfhost/linter.no`
- `selfhost/json.no`
- `selfhost/lexer/lexer_m1.no`
- testfiler

Desse kan også innehalde same mønster, men dei er ikkje den mest direkte forklaringa på dagens raude bundle/refresh-bane.

## Konklusjon
Den viktigaste attverande feilen for AI-sjølvutvikling er no ikkje uklar lenger:

1. aktiv bundle/refresh-bane er framleis ikkje frisk
2. mønsterklassen som peikar seg ut er framleis `[]` + `legg_til(...)` + vidare bruk
3. neste beste patchrekkefølgje er:
   - `selfhost/nc_main.no`
   - `selfhost/common.no`
   - `selfhost/elf_compile_driver.no`
   - `selfhost/selfcompile_l5*.no`
   - deretter breiare herdning i `vm.no` / `lsp/server.no`

## Status for AI-sjølvutvikling
Så lenge denne blokka står att:
- `ai_self_development_ready = false`
