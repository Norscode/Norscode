# IR-kontrakt v1

Dette er den offisielle teksten for `selfhost.ir_contract`.

Målet er å ha én stabil kontrakt for den lille IR-flaten som brukes av selfhost,
testene og compiler-bridge-laget.

## Versjon

```text
selfhost-ir-contract-v1
```

## Format

Alle IR-programmer i denne kontrakten representeres som linjer på formen:

```text
<pc>: <opcode> [argument]
```

For opcodes uten argument skrives bare opkoden.

## Kanoniske programer

### Add + print

```text
0: PUSH 3
1: PUSH 4
2: ADD
3: PRINT
4: HALT
```

### Push + halt

```text
0: PUSH 3
1: HALT
```

### Impliserer høyre

Dette er den høyrevendte implication-sekvensen som selfhost bruker i IR-kontrakten.

```text
0: PUSH 1
1: PUSH 0
2: SWAP
3: NOT
4: SWAP
5: OR
6: PRINT
7: HALT
```

### Impliserer venstre

```text
0: PUSH 1
1: PUSH 0
2: NOT
3: OR
4: PRINT
5: HALT
```

## Strict-feil

### Ukjent opcode

```text
/* feil: ukjent opcode FOO ved token 0 */
```

### Ugyldig heltall

```text
/* feil: ugyldig heltallsargument 03 ved token 1 */
```

### Manglende argument

```text
/* feil: mangler argument for PUSH ved token 0 */
```

## Implementasjon

Kanonisk implementasjon ligger i:

- [`selfhost/ir_contract.no`](../selfhost/ir_contract.no)

Testdekning for kontrakten ligger i:

- [`tests/test_ir_contract.no`](../tests/test_ir_contract.no)
