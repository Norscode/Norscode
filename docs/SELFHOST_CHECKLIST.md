# Norscode Selfhost Checklist

## Core Goal

Norscode compiler
 -> builds Norscode compiler
 -> without Python

---

# Repository Stability

- [ ] Cleanup scripts stable
- [ ] No tmp/debug files in repo
- [ ] Deterministic builds
- [ ] Stable folder structure
- [ ] Release artifacts separated

---

# Compiler Stability

- [ ] Stable lexer
- [ ] Stable parser
- [ ] Stable AST
- [ ] Stable semantic analysis
- [ ] Stable backend generation
- [ ] Stable optimizer

---

# Bootstrap Stability

- [ ] bootstrap_verify.sh passes
- [ ] compiler_a == compiler_b
- [ ] compiler_b == compiler_c
- [ ] Regression tests stable

---

# Python Migration

- [ ] Helper scripts removed
- [ ] Build tooling migrated
- [ ] Package tooling migrated
- [ ] Runtime tooling migrated
- [ ] Backend migrated
- [ ] Semantic migrated
- [ ] Compiler core migrated

---

# Runtime Stability

- [ ] File I/O stable
- [ ] HTTP runtime stable
- [ ] Async runtime stable
- [ ] Concurrency stable
- [ ] Memory safety verified

---

# Tooling Ecosystem

- [ ] Package manager
- [ ] Formatter
- [ ] Language server
- [ ] Test runner
- [ ] Debugger

---

# Long-Term Goals

- [ ] Stable Norscode Studio
- [ ] Stable package registry
- [ ] Native backend
- [ ] Enterprise runtime
