# Selfhost Fase 3 - IR til bytecode v1

Dette dokumentet skisserer den første stabile IR-til-bytecode-løypa i fase 3.

## Formål

- Gjere overgang frå semantic/IR til bytecode enkel å følgje
- Halde output deterministisk og lett å teste
- Sikre at NCB JSON er den normale outputflata

## Hovudflyt

Den normale løypa er:

1. AST blir validerte og analysert
2. Semantic-resultat blir brukt som grunnlag for generering
3. Bytecode blir emitert som NCB JSON
4. Output blir køyrd i `selfhost/vm.no`

## Inngang og utgang

- Inngang: AST-node eller ir-representasjon frå parsar/semantic
- Utgang: `norscode-bytecode-v1` JSON

## Støttefunksjonar

`selfhost/compiler/ir_to_bytecode.no` inneheld allereie ei rekkje JSON-hjelparar som gjer løypa tydeleg:

- escaping av tekst
- serialisering av lister og mappar
- JSON-instruksjonar med tekst, heiltal og bool
- normalisering av web-schema og eksempel

## Kjerneprinsipp

- Ingen Python-steg i normal veg
- Ingen C-steg i normal veg
- Bytecode-formatet skal vere maskinlesbart og stabilt
- Manglande input skal handterast eksplisitt

## Praktisk status

- Fila er allereie strukturert som bytecode-kompilator
- JSON-hjelparane ligg på plass
- Dokumentet for fase 3 skal halde linja for vidare utviding

## Vidare arbeid

- Gjere IR-innhaldet meir eksplisitt i dokumentasjonen
- Leggje til minimale smoke-reglar for generering
- Kople IR-løypa tydelegare til status og regresjonar
