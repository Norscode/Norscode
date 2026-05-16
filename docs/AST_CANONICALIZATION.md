# AST Canonicalization

## Goal

Normalize AST dumps into deterministic and reproducible representations.

---

# Why AST Canonicalization Matters

Bootstrap verification depends on:
- deterministic AST traversal
- stable serialization
- reproducible compiler stages

Without canonicalization:
- AST ordering may diverge
- bootstrap hashes may differ
- compiler equivalence may fail

---

# Canonical AST Tool

Use:

bash tools/ast_canonical_dump.sh <input_ast_dump> <output_dump>

---

# Canonicalization Rules

AST dumps should:
- normalize whitespace
- normalize indentation
- use stable ordering
- avoid timestamps/random IDs
- avoid runtime-specific metadata

---

# Verification Goals

Canonical AST dumps should produce:
- deterministic hashes
- stable ordering
- reproducible serialization

---

# Strategic Purpose

AST canonicalization enables:
- bootstrap debugging
- deterministic verification
- compiler equivalence analysis
- regression stability

---

# Long-Term Goal

A fully deterministic and reproducible AST pipeline.
