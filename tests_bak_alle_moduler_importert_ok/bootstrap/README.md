# Bootstrap Stabilization Tests

## Goal

Verify that the Norscode compiler bootstrap chain
is deterministic and reproducible.

---

# Core Verification

compiler_a
 -> compiler_b
 -> compiler_c

Compiler outputs should be identical.

---

# Required Areas

Bootstrap tests should validate:
- deterministic AST ordering
- deterministic symbol ordering
- deterministic backend generation
- deterministic IR transforms
- compiler equivalence

---

# Important Rule

Every bootstrap failure should:
- generate trace output
- generate deterministic dumps
- produce a permanent regression test

---

# Recommended Debug Flags

- --dump-ast
- --dump-ir
- --dump-symbols
- --backend-trace
- --bootstrap-trace

---

# Strategic Purpose

These tests protect against:
- bootstrap regressions
- hidden nondeterminism
- unstable compiler stages
- unstable backend ordering

---

# Long-Term Goal

A stable self-hosted compiler with reproducible builds.
