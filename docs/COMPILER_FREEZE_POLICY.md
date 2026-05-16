# Norscode Compiler Freeze Policy

## Current Project Phase

Norscode is currently in the selfhost stabilization phase.

Primary goal:
- deterministic compiler builds
- stable bootstrap chain
- reduced Python dependencies
- self-hosted compiler

---

# Feature Freeze Rules

Until bootstrap stability is achieved, do not add:
- new language keywords
- new syntax patterns
- experimental parser behavior
- gateway features
- studio features
- AI integrations
- OS or kernel features

---

# Allowed Changes

Allowed work:
- compiler correctness fixes
- bootstrap stabilization
- deterministic build fixes
- regression tests
- semantic cleanup
- backend stabilization
- Python dependency reduction
- compiler architecture cleanup

---

# Deterministic Build Requirement

All compiler changes must preserve deterministic builds.

---

# Regression Requirement

Every compiler or runtime bug must produce a permanent regression test.

---

# Bootstrap Stability Priority

Correctness is prioritized over:
- performance
- features
- syntax additions
- optimization passes

---

# Long-Term Goal

A stable self-hosted compiler platform with reproducible builds.
