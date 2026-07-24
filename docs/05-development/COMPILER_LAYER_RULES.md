# Compiler Layer Rules

## Goal

Enforce deterministic and isolated compiler architecture
inside the Norscode compiler pipeline.

---

# Allowed Dependency Direction

lexer
 -> parser
 -> ast
 -> semantic
 -> ir
 -> optimizer
 -> backend
 -> bootstrap

Dependencies should not move backward.

---

# Parser Rules

The parser layer must not:
- depend on backend logic
- depend on runtime deployment systems
- depend on package tooling

---

# Semantic Rules

The semantic layer must not:
- perform backend generation
- perform deployment logic
- depend on gateway/runtime services

---

# IR Rules

The IR layer must:
- remain backend-independent
- remain deterministic
- support reproducible serialization

---

# Backend Rules

The backend layer must not:
- perform parsing
- perform semantic resolution
- use nondeterministic ordering

---

# Bootstrap Rules

The bootstrap layer must:
- orchestrate deterministic stages
- verify compiler equivalence
- preserve reproducible artifacts
- avoid runtime-specific behavior

---

# Strategic Purpose

Layer isolation protects:
- bootstrap stability
- compiler correctness
- deterministic builds
- self-host validation

---

# Long-Term Goal

A fully deterministic and self-hosted compiler architecture.
