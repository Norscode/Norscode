# Bootstrap Diff Analysis

## Goal

Detect and analyze deterministic differences between compiler bootstrap stages.

---

# Why Diff Analysis Matters

When:

compiler_a
 -> compiler_b
 -> compiler_c

produces different outputs, the compiler is not yet fully deterministic.

Diff analysis helps identify:
- unstable backend ordering
- semantic ordering instability
- nondeterministic transforms
- unstable serialization

---

# Diff Analyzer Tool

Use:

bash tools/bootstrap_diff_analyzer.sh <build_a> <build_b>

---

# Verification Goals

The analyzer should help verify:
- compiler equivalence
- deterministic bootstrap stages
- reproducible builds
- stable backend generation

---

# Common Failure Sources

- unordered symbol traversal
- unstable AST ordering
- nondeterministic IR transforms
- dependency ordering instability
- timestamps/random metadata

---

# Strategic Purpose

Bootstrap diff analysis accelerates:
- bootstrap stabilization
- regression debugging
- deterministic verification
- self-host validation

---

# Long-Term Goal

A fully deterministic and reproducible bootstrap compiler chain.
