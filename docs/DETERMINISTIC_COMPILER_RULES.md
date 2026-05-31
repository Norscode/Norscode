# Deterministic compiler rules

Denne spesifikasjonen definerer regler som må oppfylles for at Norscode skal kunne:

- bli full selfhosted
- produsere reproducerbare builds
- eliminere eldre bootstrap
- støtte native backend

---

# Hovedmål

Samme input:

```text
same source
```

må alltid produsere:

```text
same tokens
same AST
same IR
same bytecode
same binary
```

uavhengig av:

- maskin
- operativsystem
- tidspunkt
- compile order

---

# 1. Token determinism

Tokenizer må:

- produsere identisk token-sekvens
- bruke stabil keyword-mapping
- ha deterministiske source-posisjoner

---

# 2. Parser determinism

Parser må:

- ha stabil precedence
- ha stabil associativity
- aldri generere tilfeldige noder
- aldri bruke unordered traversal

---

# 3. AST determinism

AST må:

- serialiseres stabilt
- traverseres stabilt
- være diffbar

---

## Ikke tillatt

```text
unordered map iteration
random node ids
non-stable traversal
```

---

# 4. IR determinism

IR lowering må:

- generere identiske labels
- generere identiske jumps
- generere identisk operator-lowering

---

# 5. Backend determinism

Backend må:

- ha stabil instruction ordering
- ha stabil symbol ordering
- ha stabil stack lowering

---

# 6. SWAP rules

## Kritisk regel

SWAP må aldri introduseres tilfeldig.

SWAP skal kun kunne oppstå:

- via eksplisitt backend lowering
- via optimizer

Ikke i:

- parser
- AST builder
- precedence handling

---

# 7. Snapshot parity

Alle compiler-steg må kunne snapshots.

---

## Snapshot-typer

```text
/source
/tokens
/ast
/ir
/bytecode
```

---

# 8. Reproducible builds

To compiler-kjøringer må produsere:

```text
identical output hashes
```

---

# 9. CI requirements

CI må sjekke:

```bash
norcode selfhost-parity --suite all
```

og:

```bash
norcode ci --require-selfhost-ready
```

---

# 10. Compiler chain verification

Målet:

```text
compiler.no
 ↓
compiler
 ↓
compiler.no
```

må gi:

```text
identical compiler output
```

---

# 11. Forbidden instability

Ikke tillatt:

- tilfeldig AST-order
- tilfeldig symbol-order
- tilfeldig register-order
- tilfeldig hash-order
- forskjellig compile output mellom kjøringer

---

# 12. Long-term importance

Determinism er nødvendig for:

- selfhosting
- native backend
- distributed builds
- package ABI
- caching
- reproducible releases
- secure verification

---

# Definition of done

Norscode er deterministisk når:

```text
same source
=
same compiler output
```

100% av tiden.
