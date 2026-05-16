# AST and IR Debugging Strategy

## Goal

Provide deterministic visibility into compiler internals
for bootstrap stabilization and self-host verification.

---

# Required Debug Systems

## AST Dump

Recommended flag:

--dump-ast

Purpose:
- inspect parser output
- inspect AST ordering
- detect nondeterministic structures

---

## IR Dump

Recommended flag:

--dump-ir

Purpose:
- inspect compiler transforms
- inspect backend input
- verify deterministic IR generation

---

## Symbol Table Dump

Recommended flag:

--dump-symbols

Purpose:
- inspect scope ordering
- inspect symbol resolution
- inspect import resolution

---

## Backend Trace

Recommended flag:

--backend-trace

Purpose:
- inspect backend ordering
- inspect bytecode generation
- inspect dependency traversal

---

# Critical Rule

All dumps must:
- use stable ordering
- use canonical serialization
- avoid timestamps/random IDs

---

# Strategic Purpose

These debugging systems are required for:
- bootstrap debugging
- deterministic verification
- regression analysis
- compiler equivalence verification

---

# Long-Term Goal

A fully deterministic and inspectable self-hosted compiler platform.
