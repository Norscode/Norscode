# V8000 Refresh and bundle recheck

## Resultat
Etter `v7800` source-side herdning vart tre ting sjekka på nytt.

### 1. Full bootstrap refresh
Kommando:
- `NORSCODE_V6000_MODE=--apply ./bin/nc run tools/refresh_bootstrap_compiler_v6000.no`

Resultat:
- framleis raud
- stoppar i:
  - `NORSCODE_REGEN_BOOTSTRAP_FULL=1 ./bin/nc run tools/nc_regen_bootstrap.no`
  - `norscode: Stack underflow`

### 2. Direkte single-module bundler-kall med liste-literal i miljøvegen
Kommando:
- `NORSCODE_CMD=run NORSCODE_FILE=selfhost/bundler.no NORSCODE_BUNDLE_ARGS='selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no'`

Resultat:
- framleis raud
- `norscode: Stack underflow`

### 3. Split-bygd modular-liste inn i `bygg_bundle(...)`
Kommando:
- `tests/fixtures/v7600/bundler_single_module_split_v7600.no`

Resultat:
- grøn
- bundle vart skrive ut som venta

## Tolkning
Dette gjer skiljet veldig klart:
- `bygg_bundle(...)` fungerer når modul-lista kjem frå trygg variabelbane
- aktiv direkte bundle-bruk via miljø/kallsti er framleis sårbar
- full bootstrap refresh er framleis blokkert i same gamle regenereringsløype

## Konklusjon
`v7800` gjorde source-sida betre, men den eine store attverande blokkeringa er no:
- aktiv bootstrap refresh / direkte bundle-inngang som framleis treffer `Stack underflow`

Det betyr at neste rette steg ikkje er meir generell source-rydding, men å isolere korleis `NORSCODE_BUNDLE_ARGS` og direkte bundle-inngang blir materialisert før `bygg_bundle(...)` får kontroll.
