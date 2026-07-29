# V4800 codegen-diagnose status

## Samandrag

`v4800` gjer compile-blokkeringa frå `v4600` konkret og reproduserbar.

Parseren er grøn for liste-, struct- og indeksuttrykk, men aktiv compile-løype
droppar framleis bytecode for desse uttrykka når dei står i `la`-initialiseringar.
Det er dette som seinare slår ut som `Stack underflow` i
`archive/legacy_c_backend/ncb_to_c.no`.

## Verifisert grønt

- AST-probe viser korrekt parse for:
  - `La(liste)[Liste[Heltall(1),Heltall(2),Heltall(3)]]`
  - `La(outer)[StructLiteral[StructPar(functions)[Heltall(1)]]]`
  - `La(y)[Indeks[Ident(liste),Heltall(1)]]`
- Enkel heiltals-initialisering er framleis grøn:
  - `la x = 1` gir `["PUSH_CONST",1]` etterfølgt av `["STORE_NAME","x"]`

## Verifisert raudt

Compile-output for `tests/fixtures/literal_index_probe_v4800.no` manglar framleis:

- `BUILD_LIST`
- `BUILD_MAP`
- `INDEX_GET`

Faktisk output hoppar direkte til:

```text
["STORE_NAME","liste"]
["STORE_NAME","outer"]
["STORE_NAME","y"]
```

utan å pushe verdiuttrykka først.

## Viktige observasjonar

1. Parseren er ikkje problemet i denne fasen.
2. Semantic-laget muterer ikkje AST-en og ser ikkje ut til å vere årsaka.
3. `nc compile` går gjennom `selfhost/nc_main.no -> selfhost/kompiler.no`, så dette er ikkje berre ein shell-wrapper-feil.
4. Direkte kall mot djup selfhost-API:

```text
selfhost.compiler.ir_to_bytecode.kompiler_til_ncb_json
```

feilar i aktiv runtime med:

```text
Ukjent selfhost API: selfhost.compiler.ir_to_bytecode.kompiler_til_ncb_json
```

Det tyder på at det framleis finst eit skilje mellom kjeldekoden vi les og den
faktiske selfhost-API-/compile-løypa som er tilgjengeleg i aktiv binær.

## Repro

- Kjeldeprobe:
  - `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tests/fixtures/literal_index_probe_v4800.no`
- Diagnose-script:
  - `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tools/run_codegen_diagnose_v4800.no`

## Status

Denne fasen er medvite ikkje markert grøn. `v4800` er ein presis blokk-status,
ikkje ein falsk “vidare kanskje”.

Neste rette store mål er:

`v4801-v5000`: lukke skiljet mellom AST/emitter-kjelde og aktiv compile-løype,
slik at `BUILD_LIST`, `BUILD_MAP` og `INDEX_GET` faktisk kjem ut i NCB igjen.
