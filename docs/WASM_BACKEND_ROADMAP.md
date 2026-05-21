# Norscode WASM Backend Roadmap

## Goal

Compile Norscode to WebAssembly.

---

# Planned Pipeline

```text
Norscode
 -> AST
 -> Semantic Analysis
 -> IR
 -> WASM Backend
 -> WebAssembly Module
```

---

# Benefits

- browser execution
- sandboxing
- portability
- high performance
- future web IDE

---

# Required Components

- stable IR
- deterministic optimizer
- memory model
- runtime ABI
- source maps

---

# Planned Milestones

## Phase 1

- minimal WASM emitter
- integer math
- functions
- variables

## Phase 2

- strings
- memory management
- lists
- modules

## Phase 3

- async runtime
- browser integration
- debugger support
- source maps

## Phase 4

- JIT experiments
- WASI support
- native embedding
