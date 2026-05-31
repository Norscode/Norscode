# Selfhost compiler chain

Denne spesifikasjonen beskriver hele compiler-kjeden som kreves for å gjøre Norscode:

- full selfhosted
- historisk vei-uavhengig
- deterministisk
- standalone

---

# Hovedmål

Fra:

```text
historisk vei
 ↓
Norscode compiler
```

Til:

```text
Norscode compiler
 ↓
Norscode compiler
```

uten ekstern bootstrap-runtime.

---

# Compiler chain nivåer

## Stage 0 — Bootstrap compiler

Dette er dagens situasjon.

```text
historisk vei runtime
 ↓
Bootstrap compiler
 ↓
Norscode program
```

---

## Stage 1 — Selfhost parser

Mål:

- tokenizer parity
- parser parity
- AST parity

Compiler frontend må være:

```text
100% deterministic
```

---

## Stage 2 — Selfhost IR

Mål:

- identisk lowering
- identisk labels
- identisk jumps
- identisk operator lowering

---

## Stage 3 — Selfhost optimizer

Optimizer-pass flyttes til Norscode.

Eksempler:

- constant folding
- dead code elimination
- bool normalization
- redundant SWAP cleanup

---

## Stage 4 — Selfhost backend

Backend må kunne generere:

- bytecode
- C backend
- senere native backend

uten historisk vei.

---

## Stage 5 — Selfhost runtime

Runtime må være separat fra compiler.

Runtime ansvar:

- strings
- memory
- IO
- networking
- async
- collections

---

## Stage 6 — Compiler compiles compiler

Kritisk milestone:

```bash
norcode build compiler.no
./compiler build compiler.no
```

må gi:

```text
identical compiler output
```

---

# Determinism requirements

Samme source må alltid gi:

```text
same tokens
same AST
same IR
same bytecode
same binary
```

---

# Snapshot verification

Alle compiler-lag må snapshots:

```text
/source
/tokens
/ast
/ir
/bytecode
/binary
```

---

# Forbidden instability

Ikke tillatt:

- tilfeldig traversal
- tilfeldig symbol-order
- tilfeldig register-order
- tilfeldig hash-order
- forskjellige compiler builds

---

# Runtime ABI

Backend skal targete stabil runtime ABI.

Eksempel:

```text
string_alloc
list_alloc
map_alloc
io_write
socket_open
```

---

# CI requirements

Følgende må alltid være grønt:

```bash
norcode selfhost-parity --suite all
norcode ci --require-selfhost-ready
```

---

# Bootstrap elimination

historisk vei kan fjernes når:

- compiler chain er stabil
- compiler builds compiler
- output er deterministisk
- runtime er separat
- binaries er standalone

---

# Long-term architecture

Når compiler chain er stabil:

- native backend blir enklere
- distributed builds blir mulig
- IDE tooling blir stabilt
- AI tooling blir enklere
- package ABI blir mulig
- OS/runtime-lag blir mulig

---

# Definition of done

Norscode er ekte selfhosted når:

```text
Norscode compiler
 builds
Norscode compiler
```

uten historisk vei.
