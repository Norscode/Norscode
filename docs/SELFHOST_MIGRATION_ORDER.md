# Selfhost Migration Order

## Goal

Provide a safe and deterministic migration order
for removing Python dependencies from the Norscode compiler.

---

# Migration Priority

## Phase 1 — Compiler Isolation

Establish deterministic compiler layers:
- parser
- ast
- semantic
- ir
- backend
- bootstrap

---

## Phase 2 — Semantic Extraction

Move semantic analysis into isolated deterministic modules.

Priority:
- symbols
- scopes
- imports
- type checking
- generics

---

## Phase 3 — Parser Stabilization

Stabilize:
- token parsing
- precedence handling
- AST generation
- syntax diagnostics

---

## Phase 4 — IR Stabilization

Stabilize:
- IR serialization
- deterministic transforms
- optimization ordering
- backend-independent representation

---

## Phase 5 — Backend Stabilization

Stabilize:
- backend ordering
- bytecode generation
- deterministic serialization
- runtime integration

---

## Phase 6 — Bootstrap Selfhost Verification

Verify:

compiler_a
 -> compiler_b
 -> compiler_c

All compiler stages must be reproducible.

---

## Phase 7 — Python Core Removal

Remove remaining Python dependencies from:
- semantic layer
- parser layer
- backend layer
- bootstrap layer

---

# Strategic Rule

Do not add major new language/runtime features
until deterministic self-host verification is stable.

---

# Long-Term Goal

A fully deterministic and self-hosted Norscode compiler platform.
