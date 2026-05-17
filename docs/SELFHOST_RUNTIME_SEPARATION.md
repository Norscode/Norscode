# Selfhost runtime separation

Denne spesifikasjonen beskriver hvordan Norscode skal separere:

- compiler
- runtime
- bootstrap
- backend

for å eliminere Python-avhengigheter.

---

# Hovedmål

Fra:

```text
Python runtime
 ↓
Compiler
 ↓
Runtime
```

Til:

```text
Norscode compiler
 ↓
Norscode runtime
 ↓
Standalone binaries
```

---

# Problem i dagens modell

Per nå er flere lag koblet sammen:

- parser
- runtime
- compile orchestration
- backend
- CLI

Dette gjør:

- debugging vanskelig
- selfhosting ustabilt
- parity vanskelig
- native backend vanskelig

---

# Målarkitektur

```text
CLI
 ↓
Compiler frontend
 ↓
AST
 ↓
IR
 ↓
Optimizer
 ↓
Backend
 ↓
Runtime ABI
 ↓
Binary
```

---

# 1. Compiler responsibilities

Compiler skal kun gjøre:

- tokenizer
- parser
- AST
- semantic analysis
- IR lowering
- optimizer
- backend emission

---

# 2. Runtime responsibilities

Runtime skal håndtere:

- memory
- strings
- collections
- IO
- networking
- async
- filesystem

---

# 3. Forbidden coupling

Compiler skal ikke:

- eie runtime state
- eie sockets
- eie async scheduler
- eie file handles

---

# 4. Runtime ABI

Runtime må eksponere:

```text
string_alloc
list_alloc
map_alloc
io_write
socket_open
```

via stabil ABI.

---

# 5. Backend requirements

Backend skal kunne:

- targete runtime ABI
- generere standalone binaries
- generere bytecode
- senere generere native machine code

---

# 6. Bootstrap elimination

Python skal fases ut i denne rekkefølgen:

## Fase A

- parser parity
- AST parity

## Fase B

- IR parity
- bytecode parity

## Fase C

- selfhost optimizer
- selfhost backend

## Fase D

- standalone runtime
- compiler builds compiler

---

# 7. Runtime determinism

Runtime må være:

- deterministisk
- stabil
- diffbar
- cachebar

---

# 8. Debugging requirements

Alle lag må kunne inspiseres separat:

```bash
norcode debug
norcode ir-disasm
norcode selfhost-parity
```

---

# 9. Long-term direction

Når separation er ferdig:

- native backend blir enklere
- distributed builds blir mulig
- IDE tooling blir enklere
- AI tooling blir enklere
- package ABI blir stabil

---

# 10. Critical milestone

Runtime separation er vellykket når:

```bash
./compiler build compiler.no
```

fungerer uten:

- Python runtime
- Python orchestration
- Python parser

---

# Definition of done

Runtime separation er ferdig når:

- compiler er selvstendig
- runtime er separat
- backend targeter runtime ABI
- binaries kjører standalone
- Python ikke er nødvendig i compile path
