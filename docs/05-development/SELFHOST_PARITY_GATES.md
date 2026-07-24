# Selfhost parity gates

Denne spesifikasjonen definerer obligatoriske gates som må passeres før:

- eldre bootstrap kan fjernes
- compiler kan regnes som selfhosted
- native backend kan stabiliseres

---

# Hovedmål

Alle compiler-lag må være:

```text
identiske
```

mellom:

- bootstrap compiler
- selfhost compiler

---

# Gate 1 — Token parity

## Krav

Samme source må gi:

```text
same tokens
```

---

## Verifisering

```bash
norcode debug test.no --tokens
```

---

## Feil som ikke er tillatt

- keyword mismatch
- operator mismatch
- position mismatch
- token ordering mismatch

---

# Gate 2 — AST parity

## Krav

Samme source må gi:

```text
same AST
```

---

## Verifisering

```bash
norcode debug test.no --ast --json
```

---

## Ikke tillatt

- forskjellig precedence
- forskjellig associativity
- forskjellig nesting
- unstable traversal

---

# Gate 3 — IR parity

## Krav

Samme AST må gi:

```text
same IR
```

---

## Verifisering

```bash
norcode ir-disasm file.nlir --diff
```

---

## Ikke tillatt

- label mismatch
- jump mismatch
- lowering mismatch

---

# Gate 4 — Bytecode parity

## Krav

Samme IR må gi:

```text
same bytecode
```

---

## Ikke tillatt

```text
SWAP divergence
missing AND
missing OR
```

---

# Gate 5 — Runtime parity

## Krav

Samme bytecode må gi:

```text
same runtime behavior
```

på:

- bootstrap runtime
- selfhost runtime

---

# Gate 6 — Deterministic builds

## Krav

To builds av samme source må gi:

```text
identical hashes
```

---

# Gate 7 — Compiler recursion stability

## Krav

Compiler må kunne:

```text
compile compiler
```

uten:

- recursion overflow
- parser loops
- stack corruption

---

# Gate 8 — Compiler chain verification

## Kritisk milestone

```bash
norcode build compiler.no
./compiler build compiler.no
```

må produsere:

```text
identical compiler output
```

---

# Gate 9 — Bootstrap elimination

historisk vei kan fjernes når:

- token parity = 100%
- AST parity = 100%
- IR parity = 100%
- bytecode parity = 100%
- runtime parity = 100%
- deterministic builds = 100%
- compiler chain verification = 100%

---

# CI enforcement

Følgende må være grønt:

```bash
norcode selfhost-parity --suite all
norcode ci --require-selfhost-ready
```

---

# Definition of done

Selfhost parity gates er bestått når:

```text
bootstrap compiler
=
selfhost compiler
```

for alle compiler-lag.
