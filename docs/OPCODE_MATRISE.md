# Opcode-støttematrise (tolk vs AOT)

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
| BUILD_LAMBDA | JA |  . | JA |
| BUILD_LIST | JA | JA | JA |
| BUILD_MAP | JA | JA | JA |
| CALL | JA | JA | JA |
| CALL_VALUE | JA |  . | JA |
| CLEAR_PENDING | JA |  . |  . |
| COMPARE_EQ | JA | JA | JA |
| COMPARE_GE | JA | JA | JA |
| COMPARE_GT | JA | JA | JA |
| COMPARE_LE | JA | JA | JA |
| COMPARE_LT | JA | JA | JA |
| COMPARE_NE | JA | JA | JA |
| DIV |  . |  . | JA |
| DUP | JA | JA | JA |
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

Totalt opcodar: 57 · tolk-støtta men ikkje full AOT: 16
[vm] entry returned
