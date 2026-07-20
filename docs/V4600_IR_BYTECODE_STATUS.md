# V4600 IR / Bytecode Status

Dato: 2026-06-25
Fase: v4401-v4600 IR/bytecode stabilisering

## Sluttstatus

Fasen er delvis grøn og delvis blokkert.

## Grønt

Compile-gate for store selfhost-filer er grøn på macOS aktiv `dist`:
- `selfhost/vm.no`
- `selfhost/bundler.no`
- `selfhost/nc_main.no`

IR-runtime-fixturen viser også at desse delane verkar:
- løkkelabels med `fortsett` og `bryt`
- `kast(...)` + `prøv/fang` i runtime-bana

Verifisert output:
- `loop_test OK: 8`
- `try_fang OK: fanga:boom`

## Raudt / blokkert

`returner` utan verdi er framleis feil i aktiv native compile/run-bane.

Fixturen viser framleis:
- `SKAL_IKKJE`

Det betyr at uoppnåeleg kode etter `returner` faktisk blir køyrd i aktiv binær.

## Kva som vart fiksa i kjelde

Parseren er stramma inn i:
- `/Users/jansteinar/Norscode AI/prosjekter/Norscode/selfhost/parser.no`

Ny regel:
- `parse_returner(...)` tek berre med uttrykk dersom det står på same linje som `returner`.

Fixturen er også retta til å teste ekte throwable-bane med `kast(...)` i staden for `feil(...)`:
- `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tests/fixtures/ir_bytecode_runtime_v4600.no`

## Hovudblokkering

Den aktive native/stage0-binæren fekk ikkje parserfiksen inn, fordi native rebuild-løypa stoppar i `ncb_to_c.no` med:
- `norscode: Stack underflow`

Det råkar både:
- `tools/build_native_candidate_v3002.no`
- direkte `./bin/nc run archive/legacy_c_backend/ncb_to_c.no`

## Konklusjon

`v4600` har no gitt oss ein presis og reproducerbar blokkering:
- compile av store selfhost-filer: grøn
- try/fang + loop labels: grøn
- `returner` utan verdi i aktiv native-bane: raud
- native rebuild/codegen via `ncb_to_c`: raud

## Neste rette steg

Neste store bør vere å lukke denne rebuild/codegen-blokkeringa som eige steg:
- `v4601-v4800` med fokus på `ncb_to_c` / native codegen / stage0 release discipline
