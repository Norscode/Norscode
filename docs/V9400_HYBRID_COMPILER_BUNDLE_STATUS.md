# V9400 compiler-seed og aktiv Norscode-bane

V9400-bundelen er no berre stage-0-seed for aktiv `dist/norscode_native`.
Kompilering, kjeldenormalisering, importgraf og bundle-fletting blir køyrde av
Norscode-verktøy. Den aktive bana brukar ikkje Node eller JavaScript.

Kjelde:
- `bootstrap/precompiled/hybrid_compiler_v9400_seed.ncb.json`
- `bootstrap/precompiled/lexer_m1.ncb.json`
- `bootstrap/precompiled/parser.ncb.json`
- `build/v3002/ir_to_bytecode_source_check4.ncb.json`

Patchar i bundelen:
- legg inn desimallesing frå den ferske lexeren
- skil `Desimal` frå `Heltall` i parseren
- skriv validerte desimalkonstantar som JSON-tal i NCB
- lappar `fresh.ir.emit_uttrykk` idempotent med desimalbana
- brukar streng `json_skriv`, slik at bytekodetekst som `"0"`, `"true"` og
  `"null"` ikkje blir endra til andre JSON-typar

Verktøy:
- `tools/build_hybrid_compiler_bundle_v9400.no` bygg stage-0-bundelen
- `tools/compile_with_hybrid_bundle_v9400.no` eig aktiv kompilering og importgraf
- `tools/bundle_with_hybrid_compiler_v9500.no` eig aktiv modular bundle-fletting

Verifisert mål:
- `ncb["entry"]` -> `INDEX_GET`
- `[1,2]` -> `BUILD_LIST 2`
- `{"x":1}` -> `BUILD_MAP 1`
- `1.5` -> `PUSH_CONST 1.5`, ikkje `null`
- Norscode-byggjaren produserer 143 funksjonar og kanonisk identisk JSON med
  den aktive v9400-bundelen
- den bygde bundelen kompilerer `selfhost/vm.no` med 175 hovudfunksjonar og 398
  funksjonar etter importfletting

Dei tidlegare JavaScript-baserte bygg-, overlay- og postprosesseringsfilene er
fjerna. Eigarskapsgaten feilar dersom dei blir kopla inn att i aktiv bane.
