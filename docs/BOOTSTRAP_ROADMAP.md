# Norscode Bootstrap Roadmap

## Primary Goal

Transition Norscode from a hybrid Python/Norscode compiler chain
into a fully self-hosted language platform.

---

# Phase 1 — Repository Stabilization

Goals:
- Remove temporary/debug files
- Stabilize repository structure
- Reduce noise in builds
- Improve deterministic behavior

Completed:
- .gitignore cleanup
- Cleanup automation script

---

# Phase 2 — Compiler Isolation

Target structure:

compiler/
  lexer/
  parser/
  ast/
  semantic/
  ir/
  optimizer/
  backend/

Goals:
- Separate compiler layers
- Reduce bootstrap instability
- Improve testability

---

# Phase 3 — Bootstrap Verification

Critical milestone:

compiler_a
 -> compiler_b
 -> compiler_c

hash(b) == hash(c)

Goals:
- Deterministic builds
- Stable self-hosting
- Regression prevention

---

# Phase 4 — Remove Python Dependencies

Removal order:

1. Helper scripts
2. Release tooling
3. Test tooling
4. Package tooling
5. Compiler bootstrap
6. Semantic/compiler core

---

# Phase 5 — Runtime Stabilization

Priority areas:
- File I/O
- Async runtime
- HTTP runtime
- Memory safety
- Concurrency

---

# Phase 6 — Tooling Ecosystem

Required tools:
- Package manager
- Language server
- Formatter
- Test runner
- Debugger

---

# Strategic Rule

Do NOT prioritize:
- AI systems
- OS/kernel
- ERP systems
- Antivirus

before:
- compiler
- runtime
- bootstrap
- tooling

are stable.
