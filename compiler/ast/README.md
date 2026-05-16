# Norscode AST Core

## Purpose

The AST layer is responsible for deterministic abstract syntax tree structures
inside the Norscode compiler pipeline.

---

# Responsibilities

The AST layer should contain:
- AST node definitions
- deterministic tree traversal
- canonical serialization support
- source mapping
- traversal utilities
- validation helpers

---

# Deterministic Requirements

AST behavior must:
- preserve stable node ordering
- avoid runtime-dependent traversal
- produce reproducible serialization
- avoid unstable identifiers

---

# Bootstrap Importance

AST determinism is critical for:
- parser correctness
- semantic stability
- compiler equivalence
- bootstrap reproducibility

---

# Planned Structure

ast/
 ├── nodes/
 ├── traversal/
 ├── serialization/
 ├── validation/
 ├── diagnostics/
 └── tracing/

---

# Long-Term Goal

A fully deterministic and self-hosted AST pipeline.
