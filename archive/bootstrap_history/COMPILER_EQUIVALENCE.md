# Compiler Equivalence Verification

## Goal

Ensure that the Norscode compiler produces deterministic and reproducible output.

---

# Verification Chain

compiler_a
 -> compiler_b
 -> compiler_c

The resulting compiler outputs should be identical.

---

# Why This Matters

Compiler equivalence is required for:
- stable self-hosting
- deterministic builds
- bootstrap correctness
- regression prevention

---

# Common Causes of Failure

- unstable symbol ordering
- nondeterministic AST traversal
- unordered dictionaries/maps
- backend ordering instability
- unstable dependency traversal
- timestamps embedded in outputs

---

# Verification Tool

Use:

bash tools/compare_compilers.sh <compiler_a> <compiler_b>

---

# Important Rule

If compiler equivalence fails:
- bootstrap is not yet stable
- historisk vei removal should pause
- new features should pause

---

# Strategic Priority

Correctness and determinism are prioritized over:
- performance
- optimization
- feature growth

---

# Long-Term Goal

A fully self-hosted Norscode compiler with deterministic and reproducible builds.
