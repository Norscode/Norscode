# Canonical IR Serialization

## Goal

Normalize compiler IR dumps into deterministic and reproducible representations.

---

# Why Canonical IR Matters

IR instability can cause:
- bootstrap divergence
- backend nondeterminism
- compiler equivalence failures
- unstable optimization passes

---

# Canonical IR Tool

Use:

bash tools/ir_canonical_dump.sh <input_ir_dump> <output_ir_dump>

---

# Canonicalization Rules

IR dumps should:
- normalize whitespace
- normalize indentation
- use stable ordering
- avoid timestamps/random IDs
- avoid runtime-specific metadata

---

# Verification Goals

Canonical IR dumps should produce:
- deterministic hashes
- reproducible backend input
- stable optimization ordering
- reproducible compiler stages

---

# Strategic Purpose

Canonical IR serialization enables:
- bootstrap debugging
- backend equivalence analysis
- deterministic verification
- regression stability

---

# Long-Term Goal

A fully deterministic and reproducible IR pipeline.
