<!-- Autogenerert av tools/opcode_matrise.no — regenerer med: ./bin/nc run tools/opcode_matrise.no -->

# Opcode-støttematrise — tolk vs AOT (Omgang 2)

Kva opcodar kvar backend handterer. `JA` = dispatch finst; `.` = manglar.
Kompilatoren kan emittere alle desse; tolk-VM-en er orakelet AOT må matche.


| opcode | TOLK | AOT-x86 | AOT-arm64 |
|---|:--:|:--:|:--:|
| ADD |  . |  . | JA |
| BINARY_ADD | JA | JA | JA |
| BINARY_AND | JA | JA | JA |
| BINARY_DIV | JA | JA | JA |
| BINARY_LSHIFT | JA | JA | JA |
| BINARY_MOD | JA | JA | JA |
| BINARY_MUL | JA | JA | JA |
| BINARY_OR | JA | JA | JA |
| BINARY_RSHIFT | JA | JA | JA |
| BINARY_SUB | JA | JA | JA |
| BINARY_XOR | JA | JA | JA |
| BUILD_LAMBDA |  . |  . |  . |
| BUILD_LIST | JA | JA | JA |
| BUILD_MAP | JA | JA | JA |
| CALL | JA | JA | JA |
| CALL_VALUE |  . |  . |  . |
| CLEAR_PENDING | JA |  . |  . |
| COMPARE_EQ | JA | JA | JA |
| COMPARE_GE | JA | JA | JA |
| COMPARE_GT | JA | JA | JA |
| COMPARE_LE | JA | JA | JA |
| COMPARE_LT | JA | JA | JA |
| COMPARE_NE | JA | JA | JA |
| DIV |  . |  . | JA |
| DUP | JA | JA |  . |
| FINALLY_END | JA |  . |  . |
| FINALLY_PUSH | JA |  . |  . |
| FINALLY_RUN | JA |  . |  . |
| HALT | JA |  . |  . |
| INDEX_GET | JA | JA | JA |
| INDEX_SET | JA | JA | JA |
| JUMP | JA | JA | JA |
| JUMP_IF_FALSE | JA | JA | JA |
| LABEL | JA | JA | JA |
| LOAD_EXCEPTION | JA |  . | JA |
| LOAD_FIELD | JA |  . |  . |
| LOAD_GLOBAL | JA | JA | JA |
| LOAD_NAME | JA | JA | JA |
| LOAD_PENDING | JA |  . |  . |
| MUL |  . |  . | JA |
| OVER | JA |  . |  . |
| PARAM |  . | JA |  . |
| POP | JA | JA | JA |
| PRINT | JA |  . |  . |
| PUSH_CONST | JA | JA | JA |
| RETURN | JA | JA | JA |
| STORE_FIELD | JA |  . |  . |
| STORE_GLOBAL | JA | JA | JA |
| STORE_NAME | JA | JA | JA |
| SUB |  . |  . | JA |
| SWAP | JA |  . |  . |
| THROW | JA | JA | JA |
| TRY_BEGIN | JA |  . | JA |
| TRY_END | JA |  . | JA |
| UNARY_BNOT | JA | JA | JA |
| UNARY_NEG | JA | JA | JA |
| UNARY_NOT | JA | JA | JA |

Totalt opcodar: 57 · tolk-støtta men ikkje full AOT: 15

## Nøkkelfunn (hol AOT må tette)

- **M3 (`BUILD_LAMBDA`, `CALL_VALUE`)** — manglar OVERALT. Tolk mista dei i rebase-mergen
  (må gjenopprettast, jf. VM-fiks), AOT har dei aldri hatt. Blokkerer closures i begge.
- **Unntak/finally** (`FINALLY_*`, `LOAD_PENDING`, `CLEAR_PENDING`, `LOAD_FIELD`,
  `STORE_FIELD`) — berre tolk. AOT-backendane manglar felt- og finally-handtering.
- **`TRY_BEGIN`/`TRY_END`/`LOAD_EXCEPTION`** — tolk + AOT-arm64, men IKKJE AOT-x86.
- **`DUP`** — tolk + x86, men ikkje arm64. **`SWAP`/`OVER`/`PRINT`/`HALT`** — berre tolk.
- `ADD`/`DIV`/`MUL`/`SUB` (berre arm64) er interne codegen-etikettar, ikkje ekte opcodar
  (falske treff frå ekstraksjonen).

Rekkefølgje for AOT-paritet: M3-opcodane (Omgang 3) → unntak/felt → per-backend-hol.
