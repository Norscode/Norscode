# Parser parity debugging workflow

Denne workflowen beskriver eksakt fremgangsmåte for å debugge avvik mellom:

- bootstrap parser
- selfhost parser

Målet er:

```text
100% deterministic parser parity
```

---

# Hovedregel

All debugging skal gjøres stegvis:

```text
Source
 ↓
Tokens
 ↓
AST
 ↓
IR
 ↓
Bytecode
```

Aldri hopp direkte til bytecode.

---

# 1. Token parity

## Kommando

```bash
norcode debug test.no --tokens
```

---

## Verifiser

Begge parsere må produsere:

- identiske token-typer
- identiske token-verdier
- identisk rekkefølge
- identiske source-posisjoner

---

## Kritiske operatorer

```text
ikkje
og
eller
```

og:

```text
mindre_enn
mindre_eller_lik
storre_eller_lik
```

---

# 2. AST parity

## Kommando

```bash
norcode debug test.no --ast --json
```

---

## Verifiser

AST må være:

- identisk
- deterministisk
- serialiserbar

---

## Kritiske noder

```text
UnaryExpression
BinaryExpression
CallExpression
CompareExpression
```

---

# 3. Precedence debugging

## Typiske problemer

### Problem

```text
ikkje a og b
```

parser ulikt.

---

## Verifiser

Forventet:

```text
AND
 ├─ NOT(a)
 └─ b
```

Ikke:

```text
NOT
 └─ AND(a, b)
```

---

# 4. IR parity

## Kommando

```bash
norcode ir-disasm file.nlir --diff
```

---

## Verifiser

- identisk lowering
- identiske labels
- identiske jumps
- identisk operator lowering

---

# 5. Bytecode parity

## IKKE tillatt

```text
SWAP divergence
missing AND
missing OR
```

---

# 6. SWAP debugging

## Kritisk regel

Parser skal aldri generere SWAP.

SWAP skal kun kunne introduseres i:

- backend
- optimizer

---

## Hvis SWAP oppstår tidlig

Da finnes:

- precedence-feil
- lowering-feil
- stack-order bug

---

# 7. Infinite recursion debugging

## Symptomer

```text
Maks steg overskredet
```

---

## Debug strategi

Logg:

- recursion depth
- instruction pointer
- AST depth
- parser state
- fallback state

---

## Vanlige årsaker

- parser fallback-loop
- precedence recursion leak
- AST re-entry

---

# 8. Determinism rules

Samme input må alltid gi:

```text
same tokens
same AST
same IR
same bytecode
```

---

# 9. Snapshot workflow

For hver testcase:

```text
/source
/tokens
/ast
/ir
/bytecode
```

lagres som snapshot.

---

# 10. Recommended development loop

## Hurtigsløyfe

```bash
norcode selfhost-parity --suite m1
norcode test tests/test_selfhost.no
```

---

## Full kontroll

```bash
norcode selfhost-parity --suite all
norcode ci --require-selfhost-ready
```

---

# 11. Definition of done

Parser parity er ferdig når:

```bash
norcode selfhost-parity --suite all
```

returnerer:

```text
100% parity
```

uten:

- SWAP divergence
- recursion overflow
- AST mismatch
- IR mismatch
- operator mismatch
