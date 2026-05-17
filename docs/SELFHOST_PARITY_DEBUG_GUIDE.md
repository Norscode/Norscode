# Selfhost parity debug guide

Denne guiden beskriver anbefalt arbeidsflyt for å feilsøke parity-avvik mellom:

- bootstrap compiler (Python)
- selfhost compiler
- IR/backend
- runtime

---

# 1. Kjør målrettet parity

## Kun M1

```bash
norcode selfhost-parity --suite m1
```

## Kun M2

```bash
norcode selfhost-parity --suite m2
```

## Full parity

```bash
norcode selfhost-parity --suite all
```

---

# 2. Debug tokens

Når operatorer feiler:

```bash
norcode debug tests/test_selfhost.no --tokens
```

Verifiser:

- `ikkje` -> NOT
- `og` -> AND
- `eller` -> OR
- `mindre_enn` -> LT
- `mindre_eller_lik` -> LE

---

# 3. Debug AST

```bash
norcode debug tests/test_selfhost.no --ast --json
```

Se etter:

- feil nesting
- feil associativity
- manglende boolske noder
- fallback til generisk expression-node

---

# 4. Sammenlign IR

```bash
norcode ir-disasm build/program.nlir --diff
```

Mål:

- identiske opcodes
- identisk stack-order
- ingen ekstra SWAP
- ingen manglende AND/OR

---

# 5. Typiske problemer

## Problem: ekstra SWAP

Årsak:

- backend forsøker å normalisere stack uten operator-kontekst

Tiltak:

- flytt stack-normalisering til IR-pass
- ikke generer SWAP direkte i parser/backend

---

## Problem: manglende AND

Årsak:

- parser returnerer unary-expression i stedet for binary-expression

Tiltak:

- verifiser precedence-tabell
- verifiser token-lookahead

---

## Problem: infinite selfhost recursion

Eksempel:

```text
Maks steg overskredet
```

Tiltak:

- logg instruction pointer
- logg AST-depth
- logg recursion depth
- identifiser fallback-loop

---

# 6. Anbefalt utviklingsflyt

## Lokal hurtigsløyfe

```bash
norcode selfhost-parity --suite m1
norcode test tests/test_selfhost.no
```

## Før commit

```bash
norcode ci --parity-suite all
```

## Før release

```bash
norcode ci --require-selfhost-ready
```

---

# 7. Readiness-mål

## Fase A

- stabil parser
- stabil tokenizer
- deterministisk bytecode

## Fase B

- compiler compiles compiler
- parity = 100%
- runtime parity

## Fase C

- native backend
- standalone binary toolchain
- Norscode bygger Norscode

---

# 8. Langsiktig arkitektur

Målarkitektur:

```text
Norscode source
    ↓
Selfhost parser
    ↓
Selfhost AST
    ↓
Selfhost IR
    ↓
Optimizer
    ↓
Backend
    ↓
Binary/runtime
```

Python skal på sikt kun være bootstrap-verktøy.
