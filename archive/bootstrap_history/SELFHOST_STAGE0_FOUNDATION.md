# Norscode Selfhost Foundation

## Main Goal

Remove historisk vei and C from the compile chain over time.

Target:

```
Norscode -> compiles Norscode
```

---

# Stage 0 — Foundation

Before removing historisk vei, the language core must become stable.

## Required Stable Systems

- lexer
- parser
- AST
- bytecode backend
- VM/runtime
- module system

---

# Recommended Repository Structure

```
/selfhost
    lexer.no
    parser.no
    ast.no
    semantic.no
    backend.no
    vm.no
```

---

# Stage 1 — Selfhost Lexer

Goal:

Replace historisk vei tokenizer with Norscode implementation.

## Required Features

### Token Types

```
IDENT
NUMBER
STRING

HVIS
ELLERS
MENS
FUNKSJON
RETURNER

PLUS
MINUS
STAR
SLASH
EQ
LT
GT
```

---

# Lexer Rules

The lexer must:

- be deterministic
- never recurse infinitely
- preserve line/column positions
- support UTF-8
- support Norwegian keywords

---

# Stage 2 — Selfhost Parser

Goal:

Generate stable AST from Norscode source.

## Parser Requirements

- deterministic precedence
- recursion guards
- syntax recovery
- explicit operator precedence
- stable AST shape

---

# AST Principles

AST nodes must:

- be serializable
- use explicit node types
- avoid runtime-specific structures
- remain backward compatible

Example:

```json
{
  "type": "if",
  "condition": {},
  "body": []
}
```

---

# Stage 3 — Bytecode Backend

Minimal opcodes only.

## Initial Opcode Set

```
PUSH_CONST
LOAD
STORE
CALL
RET
JMP
JMP_FALSE
ADD
SUB
MUL
DIV
EQ
LT
GT
```

---

# Stage 4 — VM Runtime

Initial VM goals:

- stack execution
- call frames
- local variables
- jump handling
- function calls

Avoid early complexity:

- threads
- JIT
- async runtime
- GC experiments

---

# historisk vei Removal Strategy

historisk vei must be removed module-by-module.

## Correct Strategy

```
historisk vei lexer -> Norscode lexer
historisk vei parser -> Norscode parser
historisk vei backend -> Norscode backend
```

Not:

```
Delete historisk vei immediately
```

---

# Selfhost Milestones

## Milestone 1

Norscode lexer compiles successfully.

## Milestone 2

Norscode parser generates AST.

## Milestone 3

Norscode backend emits bytecode.

## Milestone 4

Norscode compiler compiles itself.

---

# Long-Term Goal

Final architecture:

```
source
↓
lexer
↓
parser
↓
AST
↓
semantic analysis
↓
bytecode
↓
VM/native
```

Norscode becomes fully self-hosted and independent from historisk vei.
