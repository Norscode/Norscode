# Parser parity test matrix

Denne matrisen definerer minimum testdekning som kreves for:

```text
100% parser parity
```

mellom:

- bootstrap compiler
- selfhost compiler

---

# Mål

For hver test:

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

må være identisk.

---

# 1. Unary operators

## NOT

```text
ikkje sant
ikkje usant
ikkje a
ikkje ikkje a
```

---

# 2. Binary boolean operators

## AND

```text
a og b
a og b og c
(a og b) og c
```

## OR

```text
a eller b
a eller b eller c
(a eller b) eller c
```

---

# 3. Mixed boolean precedence

```text
ikkje a og b
ikkje (a og b)
a og b eller c
a eller b og c
(a eller b) og c
```

---

# 4. Comparison operators

```text
a mindre_enn b
a storre_enn b
a mindre_eller_lik b
a storre_eller_lik b
```

---

# 5. Mixed comparison + boolean

```text
a mindre_enn b og c
a mindre_enn b eller c
ikkje a mindre_enn b
```

---

# 6. Arithmetic precedence

```text
1 + 2 * 3
(1 + 2) * 3
```

---

# 7. Nested expressions

```text
(a og (b eller c))
((a og b) eller (c og d))
```

---

# 8. Parenthesis stress tests

```text
((((a))))
((a og b))
```

---

# 9. Recursion stability

Målet er:

- ingen infinite recursion
- ingen parser loops
- ingen stack overflow

---

# 10. Invalid syntax parity

Begge parsere må feile likt.

Eksempler:

```text
og a
ikkje og
(a og
```

---

# 11. Bytecode parity rules

## IKKE tillatt

```text
SWAP divergence
missing AND
missing OR
```

---

# 12. Determinism

Samme input:

```text
same source
```

må alltid gi:

```text
same AST
same IR
same bytecode
```

---

# 13. Snapshot strategy

Anbefalt snapshot-format:

```text
/source
/tokens
/ast
/ir
/bytecode
```

for hver testcase.

---

# 14. CI requirements

Før merge:

```bash
norcode selfhost-parity --suite all
```

må være grønn.

---

# 15. Definition of done

Parser parity er ferdig når:

- token parity = 100%
- AST parity = 100%
- IR parity = 100%
- bytecode parity = 100%
- deterministic output = 100%
- recursion stability = 100%
