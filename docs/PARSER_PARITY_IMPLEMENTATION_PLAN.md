# Parser parity implementation plan

Dette dokumentet beskriver konkret implementasjonsplan for:

> FASE 1 — 100% parser parity

Målet er:

```text
Source
  ↓
Python parser
  ↓
AST
```

skal være identisk med:

```text
Source
  ↓
Selfhost parser
  ↓
AST
```

for alle støttede språkfunksjoner.

---

# Kritiske problemer observert

Fra eksisterende parity-logger:

## Feil 1 — ekstra SWAP

```text
Fikk:
SWAP
NOT
SWAP
OR
```

Forventet:

```text
NOT
AND
OR
```

Indikerer:

- stack reorder bug
- feil operator lowering
- parser/backend divergence

---

## Feil 2 — precedence

Eksempel:

```text
ikkje a og b
```

tolkes forskjellig mellom:

- bootstrap parser
- selfhost parser

---

## Feil 3 — recursion overflow

```text
Maks steg overskredet
```

Tyder på:

- fallback-loop
- recursion leak
- parser state corruption

---

# FASE 1A — Token parity

## Mål

Begge parsere må produsere identiske token streams.

---

## Kritiske tokens

```text
ikkje
og
eller
mindre_enn
mindre_eller_lik
storre_eller_lik
```

---

## Legg til parity-test

```text
source → tokens → checksum
```

for:

- Python parser
- selfhost parser

Checksums må være identiske.

---

# FASE 1B — Precedence parity

## Operatorrekkefølge

Må være identisk:

```text
NOT
AND
OR
comparison
addition
multiplication
```

---

## Kritisk testmatrise

### Unary + binary

```text
ikkje a og b
ikkje (a og b)
```

### Nested

```text
(a og b) eller c
```

### Mixed compare

```text
a mindre_enn b og c
```

---

# FASE 1C — AST parity

## Mål

AST skal være:

- identisk
- deterministisk
- serialiserbar

---

## AST snapshot-format

Anbefalt:

```json
{
  "type": "BinaryExpression",
  "operator": "AND",
  "left": {},
  "right": {}
}
```

---

# FASE 1D — IR parity

Etter AST parity:

```text
AST
 ↓
IR
```

må gi identisk output.

---

# FASE 1E — Bytecode parity

Kjør:

```bash
norcode ir-disasm --diff
```

Mål:

- identiske opcodes
- identisk stack-order
- ingen ekstra SWAP

---

# Kritisk anbefaling

## Ikke generer SWAP i parser

SWAP bør:

- kun genereres i backend
- eller optimizer

Ikke i:

- precedence parser
- AST lowering

---

# Anbefalt intern struktur

```text
Tokenizer
  ↓
Pratt parser
  ↓
AST
  ↓
IR lowering
  ↓
Optimizer
  ↓
Backend
```

---

# Debugging-strategi

## Sammenlign steg for steg

### 1. Tokens

```bash
norcode debug file.no --tokens
```

### 2. AST

```bash
norcode debug file.no --ast --json
```

### 3. IR

```bash
norcode ir-disasm file.nlir --diff
```

---

# Definition of done

FASE 1 er ferdig når:

```bash
norcode selfhost-parity --suite all
```

returnerer:

```text
100% parity
```

uten:

- SWAP divergence
- AST mismatch
- recursion overflow
- operator mismatch
