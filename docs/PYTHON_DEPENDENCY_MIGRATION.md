# Python Dependency Migration Plan

## Goal

Gradually remove Python dependencies from the Norscode compiler chain
until the platform becomes fully self-hosted.

---

# Dependency Categories

## Category A — Critical Compiler Core

Highest priority.

Includes:
- parser
- semantic analysis
- optimizer
- backend generation
- bootstrap compiler

These must eventually be rewritten in Norscode.

---

## Category B — Runtime Tooling

Medium priority.

Includes:
- package tooling
- runtime generators
- build orchestration
- release tooling

Can temporarily remain hybrid.

---

## Category C — Development Tooling

Lowest priority.

Includes:
- helper scripts
- CI utilities
- diagnostics
- migration helpers

These do not block self-hosting.

---

# Recommended Migration Order

1. Helper scripts
2. Build tooling
3. Package tooling
4. Runtime tooling
5. Backend generation
6. Semantic analysis
7. Parser/compiler core

---

# Critical Rule

Do NOT remove Python modules blindly.

Every removal must be verified with:
- bootstrap verification
- regression tests
- deterministic builds

---

# Verification Requirement

After every migration:

compiler_a
 -> compiler_b
 -> compiler_c

hash(b) == hash(c)

must remain true.

---

# Strategic Focus

Priority is NOT:
- zero Python files

Priority IS:
- stable self-hosted compiler
- deterministic builds
- stable runtime

---

# Long-Term Goal

Norscode compiler
 -> builds itself
 -> maintains itself
 -> releases itself

without Python dependencies.
