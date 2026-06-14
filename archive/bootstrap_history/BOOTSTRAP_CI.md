# Bootstrap Verification CI

Verifikasjonen køyrer no som del av **`.github/workflows/ci.yml`** (Native macOS/Linux, bootstrap-self, ELF self-compile paritet, lint).
Full L1–L6-gate køyrer berre på push til `main` via **`.github/workflows/selvstendighet.yml`**.
Linux ELF self-compile parity blir køyrd som hard gate med `NC_OM6B_RUN_STAGE0=1` i begge workflowane.

## Goal

Automatically verify compiler stability and deterministic builds
on every push and pull request.

---

# Workflow

The bootstrap verification workflow performs:

1. Repository cleanup
2. active-surface / policy audit
3. Bootstrap verification on native-first path
4. Compiler equivalence verification
5. ELF self-compile parity verification on Linux x86_64

---

# Primary Goal

Ensure that:

compiler_a
 -> compiler_b
 -> compiler_c

produces deterministic compiler output.

---

# Failure Conditions

CI should fail if:
- bootstrap verification fails
- compiler equivalence fails
- compiler outputs differ
- ELF self-compile parity fails
- normal workflow drifts tilbake mot hidden fallback or maintenance-only lanes

---

# Strategic Purpose

This workflow protects against:
- bootstrap regressions
- nondeterministic compiler behavior
- unstable backend ordering
- unstable semantic ordering

---

# Long-Term Goal

A fully reproducible and self-hosted Norscode compiler platform.

---

Se også:

- [`docs/SELFHOST_CI_GATES.md`](./SELFHOST_CI_GATES.md)
