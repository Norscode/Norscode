# Norscode Compiler Pipeline

## Modern Compiler Flow

```text
Source
 -> Lexer
 -> Canonical Tokens
 -> Precedence Parser
 -> AST
 -> Semantic Analysis
 -> Typed IR
 -> SSA
 -> Optimizer
 -> Bytecode
 -> VM
```

---

# Pipeline Stages

## Lexer

Responsibilities:
- tokenization
- normalization
- keyword mapping

---

## Parser

Responsibilities:
- precedence parsing
- AST generation
- parser recovery
- incremental parsing

---

## Semantic Analysis

Responsibilities:
- type checking
- scope validation
- symbol resolution
- diagnostics

---

## Typed IR

Responsibilities:
- typed intermediate representation
- optimization groundwork
- backend abstraction

---

## SSA

Responsibilities:
- deterministic optimization
- variable versioning
- control flow optimization

---

## Optimizer

Responsibilities:
- constant folding
- dead code elimination
- IR cleanup

---

## Runtime

Responsibilities:
- deterministic execution
- tracing
- profiling
- source maps
- async scheduling
