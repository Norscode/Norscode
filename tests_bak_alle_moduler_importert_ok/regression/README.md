# Norscode Regression Tests

## Purpose

Regression tests ensure that previously fixed compiler/runtime bugs
never silently return.

---

# Recommended Areas

## Parser Tests

Verify:
- token parsing
- AST correctness
- syntax recovery
- operator precedence

---

## Semantic Tests

Verify:
- type safety
- symbol resolution
- generics
- scopes
- imports

---

## Runtime Tests

Verify:
- async behavior
- file I/O
- HTTP runtime
- memory safety
- concurrency

---

## Bootstrap Tests

Verify:
- deterministic builds
- selfhost stability
- compiler equivalence

---

# Important Rule

Every discovered bug should create:
- a permanent regression test

This prevents bootstrap instability from returning later.

---

# Long-Term Goal

Stable regression coverage
is required before:
- enterprise runtime
- package registry
- IDE ecosystem
- native backend
