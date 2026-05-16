# Deterministic Ordering Rules

## Goal

Ensure that all compiler stages behave deterministically
across bootstrap builds.

---

# Required Stable Ordering

The following systems must use deterministic ordering:

- AST traversal
- symbol tables
- dependency graphs
- backend generation
- IR transforms
- package traversal

---

# Common Failure Sources

## Unordered Maps

Avoid relying on:
- unordered dictionaries
- unordered sets
- runtime-specific iteration order

---

## Dependency Traversal

Dependency graphs must:
- use stable sorting
- resolve dependencies deterministically

---

## Backend Generation

Function generation order must:
- remain stable
- avoid nondeterministic traversal

---

# Strategic Purpose

Deterministic ordering is required for:
- bootstrap stability
- compiler equivalence
- reproducible builds
- regression verification

---

# Long-Term Goal

A deterministic self-hosted compiler chain.
