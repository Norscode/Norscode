# V4400 Selfhost Compile Status

Dato: 2026-06-25
Fase: v4201-v4400 parser og semantic rydding

## Sluttstatus

Selfhost compile-gata er grøn på:
- aktiv macOS `dist/norscode_native`
- aktiv Linux x86_64 `bootstrap/stage0/norscode-linux-x86_64`

## Verifisert grønt

Desse selfhost-filene kompilerer til gyldig NCB:
- `selfhost/lexer/lexer_m1.no`
- `selfhost/parser.no`
- `selfhost/compiler/semantic.no`
- `selfhost/compiler/ir_to_bytecode.no`
- `selfhost/kompiler.no`

Parser-feil-smoke er også verifisert på begge plattformspor med linje/kolonne:
- `Parserfeil 2:14 - forventet ) etter parameterliste, fikk NUMBER`

## Viktig kontrakt

Compile-gata må setje `NORSCODE_ROOT` eksplisitt.
Utan dette kunne importløypa slå opp modular frå feil arbeidskatalog, sjølv om runtime og compiler i seg sjølv var friske.

Dette er no bygd inn i:
- `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tools/run_selfhost_compile_phase_v4400.no`

## Probe-oppsett

- gate-script: `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tools/run_selfhost_compile_phase_v4400.no`
- parser-feil-fixture: `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tests/fixtures/compiler_error_v4400.no`
- output: `/Users/jansteinar/Norscode AI/prosjekter/Norscode/build/v4400/`

## Konklusjon

`selfhost_compile_phase=green`
