# Norscode Architecture V2

## Goal

Modernize the Norscode compiler pipeline into a stable, modular architecture.

---

# Current Problems

- Parser and bytecode generation are tightly coupled
- Token normalization is duplicated
- Selfhost recursion loops are difficult to debug
- compiler.no is too large
- Semantic analysis is incomplete
- AST is not fully separated

---

# New Compiler Pipeline

```text
Source
  -> Lexer
      -> Canonical Tokens
          -> Parser
              -> AST
                  -> Semantic Analysis
                      -> IR
                          -> Optimizer
                              -> Bytecode
                                  -> VM
```

---

# Lexer

Responsibilities:

- tokenization
- unicode normalization
- keyword mapping
- canonical token generation

Examples:

```text
mindre_enn -> TOKEN_LT
< -> TOKEN_LT
less_than -> TOKEN_LT
```

---

# Parser

Parser should:

- generate AST only
- not generate bytecode directly
- use precedence parsing

Recommended:

- Pratt parser
- Recursive descent precedence parser

---

# AST

Core node types:

- BinaryExpr
- UnaryExpr
- CallExpr
- FunctionDecl
- VariableDecl
- IfExpr
- WhileExpr
- ReturnExpr
- ModuleExpr

---

# Semantic Analysis

Responsibilities:

- scope validation
- type validation
- symbol resolution
- undefined variable detection
- return analysis

---

# IR Layer

Intermediate representation allows:

- optimization
- backend portability
- debugging
- future LLVM/WASM support

---

# Bytecode

Bytecode layer should:

- be deterministic
- avoid parser dependencies
- support disassembly
- support source mapping

---

# VM

VM goals:

- deterministic execution
- safe stack handling
- recursion guards
- step debugging

---

# Repository Structure

```text
norscode/
├── lexer/
├── parser/
├── ast/
├── semantic/
├── ir/
├── bytecode/
├── vm/
├── stdlib/
├── repl/
├── tooling/
└── tests/
```

---

# Long-term Goals

- Full selfhosting
- LLVM backend
- WASM backend
- IDE support
- LSP server
- VS Code extension
- Native package manager
- Stable registry ecosystem

---

# Immediate Priorities

1. Stable parser
2. Canonical token system
3. AST separation
4. Semantic analysis
5. Selfhost parity stability
6. VM stabilization
7. Repository cleanup

---

# Philosophy

Stability before features.

The compiler core must become deterministic and maintainable before adding additional frameworks and ecosystem features.
