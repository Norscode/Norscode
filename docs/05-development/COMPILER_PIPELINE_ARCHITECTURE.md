# Compiler pipeline architecture

Denne filen beskriver anbefalt intern arkitektur for Norscode-kompilatoren.

Målet er:

- deterministisk kompilering
- full selfhosting
- native backend
- modulær optimizer
- stabil tooling

---

# Oversikt

```text
Source
  ↓
Tokenizer
  ↓
Parser
  ↓
AST
  ↓
Semantic analysis
  ↓
IR lowering
  ↓
Optimizer
  ↓
Backend
  ↓
Binary / Bytecode
```

---

# 1. Tokenizer

Ansvar:

- lese kildekode
- produsere tokens
- håndtere Unicode
- håndtere norske nøkkelord

Eksempler:

```text
funksjon
hvis
ellers
ikkje
og
eller
```

---

# 2. Parser

Parseren skal:

- bygge deterministisk AST
- håndtere precedence korrekt
- støtte recovery/debugging
- produsere identisk AST i bootstrap + selfhost

---

# 3. AST

AST skal være:

- immutable når mulig
- serialiserbar
- diffbar
- enkel å debugge

Eksempel:

```text
BinaryExpression
  left
  operator
  right
```

---

# 4. Semantic analysis

Ansvar:

- typer
- scope
- symboltabeller
- imports
- generics senere
- visibility

---

# 5. IR lowering

AST → IR

Mål:

- backend-uavhengig representasjon
- enkel optimalisering
- deterministisk output

---

# 6. Optimizer

Første optimizer-passes:

- constant folding
- dead code elimination
- branch simplification
- boolsk normalisering
- redundant SWAP removal

Senere:

- inlining
- loop optimization
- register pressure reduction
- SSA

---

# 7. Backend

Første backends:

- C backend
- bytecode backend
- native x86_64 backend

Senere:

- ARM64
- WASM
- JIT

---

# 8. Runtime

Runtime skal være separat fra compiler.

Komponenter:

- memory
- strings
- collections
- IO
- networking
- async

---

# 9. Selfhost flow

Mål:

```text
compiler.no
   ↓
compiler
   ↓
compiler.no
```

uten historisk runtime.

---

# 10. Debugging

Kritiske debug-tools:

```bash
norcode debug
norcode ir-disasm
norcode selfhost-parity
```

---

# 11. Stabilitetskrav

## Compiler output må være:

- deterministisk
- reproducerbar
- diffbar
- cachebar

---

# 12. Langsiktig retning

Norscode skal kunne:

- bygge servere
- bygge GUI-systemer
- bygge IDE-er
- bygge AI tooling
- bygge OS-komponenter
- bygge distribuerte systemer

---

# Kritisk prioritet akkurat nå

1. Parser parity
2. AST parity
3. IR parity
4. Deterministisk bytecode
5. Full selfhost bootstrap
