# Norscode Compiler Structure

## Goal

Create a stable and modular compiler architecture
for self-hosting and long-term maintainability.

---

# Recommended Structure

compiler/
 ├── lexer/
 ├── parser/
 ├── ast/
 ├── semantic/
 ├── ir/
 ├── optimizer/
 ├── backend/
 └── runtime/

---

# Layer Responsibilities

## lexer/

Responsible for:
- tokenization
- source scanning
- lexical validation

---

## parser/

Responsible for:
- AST generation
- syntax parsing
- parser recovery

---

## ast/

Responsible for:
- AST node definitions
- AST transforms
- serialization

---

## semantic/

Responsible for:
- type checking
- symbol resolution
- scope analysis
- generics
- validation

---

## ir/

Responsible for:
- intermediate representation
- compiler transforms
- optimization preparation

---

## optimizer/

Responsible for:
- constant folding
- dead code elimination
- optimization passes

---

## backend/

Responsible for:
- bytecode generation
- native backend
- runtime integration

---

# Strategic Goal

The compiler must eventually:

Norscode compiler
 -> build Norscode compiler
 -> without Python

---

# Important Rule

Avoid mixing:
- runtime logic
- tooling logic
- parser logic
- bootstrap logic

inside the same modules.

Compiler layers should remain isolated.
