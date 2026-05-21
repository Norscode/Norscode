# Norscode LSP Roadmap

## Goal

Provide modern IDE support for Norscode.

---

# Planned Features

- syntax highlighting
- autocomplete
- diagnostics
- go to definition
- hover docs
- formatting
- semantic tokens
- rename symbol
- inline type hints

---

# Architecture

```text
VS Code
  -> LSP Client
      -> Norscode LSP Server
          -> Parser
          -> AST
          -> Semantic Analyzer
```

---

# Required Compiler Features

- stable AST
- deterministic parser
- semantic diagnostics
- symbol table
- source maps

---

# Milestones

## Phase 1

- syntax highlighting
- diagnostics
- parser tracing

## Phase 2

- autocomplete
- semantic analysis integration
- hover docs

## Phase 3

- incremental parsing
- semantic tokens
- formatting

## Phase 4

- debugger integration
- runtime tracing
- profiling
