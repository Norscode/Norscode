# V5400 Bootstrap compiler gap map

## Formål
Dette steget samlar forskjellen mellom den nye `*_real` referanse-compileren og den aktive bootstrap/generated-C compile-løypa som `nc compile` framleis brukar.

Målet er å gjere resten av arbeidet frå `v5400` og oppover mekanisk:
- bevise kva som alt verkar i referansekjeda
- peike ut kva som manglar i aktiv bootstrap-compiler
- gi ei fast rekkefølgje for migrering og re-verifisering

## Status no

`*_real`-kjeda har implementert:
- `Liste` -> `BUILD_LIST`
- `Map` -> `BUILD_MAP`
- `Indeks` -> `INDEX_GET`
- `Assignment` -> `STORE_VAR`

Aktiv bootstrap/generated-C compile-løype er framleis blokkert av at compile-output for enkle literal/index-program manglar dei tilsvarande bytekode-opera­sjonane.

## Gap-tabell

| Område | `*_real` referanse | Aktiv bootstrap/compiler | Status |
|---|---|---|---|
| Listeliteral | `BUILD_LIST` | droppar verdiuttrykk før `STORE_NAME` | raud |
| Tom liste | `BUILD_LIST` med teljar `0` | ikkje bevist grønt | raud |
| Map-literal | `BUILD_MAP` | map-verdi kjem ikkje fram i compile-output | raud |
| Indeks | `INDEX_GET` | indeksoppslag kjem ikkje fram i compile-output | raud |
| Assignment | `STORE_VAR` etter verdiuttrykk | lagring skjer utan komplett verdi-kjede | raud |
| Runtime for emitted ops | finst i referanse-VM | finst i native/runtime-lina | grøn |
| Parser for grunnuttrykk | grøn i referansekjeda | AST-probar tyder på at parse-leddet i stor grad er grønt | gul |

## Viktig skilje
Problemet er ikkje lenger primært runtime.
Problemet er heller ikkje lenger at referansekjelda manglar støtte.

Den attverande blokkeringa sit i den aktive bootstrap/generated-C compile-løypa:
- gamal lowering/emitter-kjede blir framleis brukt av `nc compile`
- eller aktiv innebygd compiler blir ikkje regenerert frå dei rette nyare kjeldene

## Foreslått migreringsrekkefølgje

1. Lukk listeliteral i aktiv bootstrap-compiler.
2. Lukk tom liste i same løype.
3. Lukk map-literal.
4. Lukk indeks.
5. Lukk assignment etter at verdiuttrykka er stabile.
6. Rerun `v4800`-reproane.
7. Rerun `v4600`-retur/stack-probar.
8. Bygg ny kandidat før noko blir promotert vidare.

## Definisjon på grønt
Bootstrap/generated-C compile-løypa kan kallast grøn når alle desse er sanne:
- compile-output for liste-probe inneheld `BUILD_LIST`
- compile-output for tom liste-probe inneheld `BUILD_LIST`
- compile-output for map-probe inneheld `BUILD_MAP`
- compile-output for indeks-probe inneheld `INDEX_GET`
- compile-output for assignment-probar inneheld både verdi-op og lagrings-op
- `ncb_to_c` sluttar å feile med `Stack underflow` for same reproduksjonsprogram

## Harde grenser
- Ikkje påstå at aktiv compile-løype er fiksa før prober viser det.
- Ikkje blande `*_real` referansegrønt med aktiv bootstrap-grønt.
- Ikkje promotere vidare berre fordi referansekjelda ser korrekt ut.
