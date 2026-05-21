# Norscode Optimizer Roadmap

## Goal

Provide deterministic compiler optimizations.

---

# Planned Optimizations

- constant folding
- dead code elimination
- inline expansion
- SSA transforms
- control flow simplification
- typed IR optimization

---

# Compiler Pipeline

```text
AST
 -> Semantic Analysis
 -> Typed IR
 -> SSA
 -> Optimizer
 -> Bytecode
```

---

# Optimization Goals

- deterministic output
- stable builds
- reproducible compilation
- future LLVM compatibility

---

# Planned Phases

## Phase 1

- constant folding
- basic IR cleanup

## Phase 2

- SSA transforms
- dead code elimination

## Phase 3

- typed optimization
- inlining
- loop optimization

## Phase 4

- backend specialization
- WASM optimization
- JIT groundwork
