# Norscode Systems Architecture

## Vision

Norscode evolves into:

- systems language
- deterministic runtime platform
- modern compiler infrastructure
- native + WASM ecosystem

---

# Compiler Pipeline

```text
Source
 -> Lexer
 -> Canonical Tokens
 -> Parser
 -> AST
 -> Semantic Analysis
 -> Type Inference
 -> Typed IR
 -> SSA
 -> CFG
 -> Optimizer
 -> Backend Lowering
 -> Bytecode / Native / WASM
```

---

# Runtime Architecture

```text
Scheduler
 -> Coroutines
 -> Async IO
 -> Sandbox
 -> Memory Pools
 -> Runtime Metrics
 -> Trace Protocol
```

---

# Safety Systems

- ownership verification
- borrow analysis
- escape analysis
- region analysis
- effect inference

---

# Tooling Vision

- semantic highlighting
- AST explorer
- CFG explorer
- optimizer visualization
- debugger integration
- runtime trace viewer

---

# Long-term Goals

- deterministic builds
- reproducible compilation
- native backend
- WASM backend
- IDE ecosystem
- educational compiler platform
