# Native backend roadmap

Denne roadmapen beskriver overgangen fra:

- Python bootstrap runtime
- hybrid compiler
- C-generering

mot:

- full selfhost compiler
- native backend
- standalone binaries

---

# Mål

Norscode skal kunne:

1. kompilere seg selv
2. bygge native binaries
3. kjøre uten Python-runtime
4. fungere som komplett backend/frontend-plattform

---

# Fase 1 — Stabil selfhost compiler

## Krav

- parser parity
- tokenizer parity
- AST parity
- IR parity
- runtime parity

## Kritiske tester

```bash
norcode selfhost-parity --suite all
norcode ci --require-selfhost-ready
```

## Mål

- deterministisk bytecode
- compiler compiles compiler

---

# Fase 2 — Stabil IR

## Legg til:

- SSA-lignende representasjon
- optimizer passes
- dead code elimination
- constant folding
- register allocation

## Resultat

- mindre binaries
- raskere runtime
- enklere native backend

---

# Fase 3 — Native codegen

## Første backend

Anbefalt:

- x86_64 Linux

Deretter:

- ARM64 macOS
- Windows x64

## Backend ansvar

- stack lowering
- register mapping
- calling convention
- memory layout
- symbol tables

---

# Fase 4 — Runtime

## Runtime-komponenter

- memory manager
- allocator
- string runtime
- collections runtime
- async runtime
- IO runtime
- networking runtime

## Senere

- JIT
- GC
- actor runtime

---

# Fase 5 — Full bootstrap

Mål:

```text
Norscode compiler
    builds
Norscode compiler
```

uten Python.

---

# Fase 6 — Distribusjon

## CLI

```bash
norcode build app.no
```

skal produsere:

```text
./build/app
```

uten ekstern runtime.

---

# Fase 7 — OS/Foundation layer

Langsiktig:

- driver APIs
- kernel abstractions
- filesystem layer
- process runtime
- networking stack

---

# Anbefalt prioritet

1. parity
2. deterministic IR
3. optimizer
4. native backend
5. standalone runtime
6. package ABI
7. plugin ABI
8. IDE protocol
9. distributed runtime

---

# Risikoer

## Høy risiko

- parser divergence
- unstable IR
- recursive compiler drift
- ABI instability

## Medium risiko

- package fragmentation
- runtime incompatibility
- backend duplication

---

# Kritisk milepæl

Når dette fungerer:

```bash
norcode build compiler.no
./compiler build compiler.no
```

har Norscode nådd ekte selfhosting.
