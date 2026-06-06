# Norscode C Backend Reference

**Status:** Archived (Fase 3+)

This document explains the C backend that was used for code generation before the native ELF emitter was developed.

## Historical Overview

In earlier phases, Norscode had this compilation pipeline:

```
source.no → ast → ir → c_generation → c_code → clang/gcc → native_binary
```

This was replaced in Fase 3 with:

```
source.no → ast → ir → ncb_json → vm.no (bytecode execution)
```

And will be further improved in Fase 6+ with:

```
source.no → ast → ir → ncb_json → elf_emitter.no → native_binary (no C needed)
```

## C Backend Components

### ncb_to_c.no (Norscode Code → C Code)

Converts NCB JSON (bytecode) to C source code.

**Example:**

Input (NCB JSON):
```json
{
  "instructions": [
    {"opcode": "PUSH", "arg": 42},
    {"opcode": "PRINT"},
    {"opcode": "HALT"}
  ]
}
```

Output (C code):
```c
int main() {
    int stack[1024];
    int sp = 0;
    
    stack[sp++] = 42;           // PUSH 42
    printf("%d\n", stack[--sp]); // PRINT
    return 0;                     // HALT
}
```

### gen_dispatch.no (VM Dispatch)

Generates C dispatch table for VM opcodes.

```c
static const void *labels[] = {
    &&L_PUSH,   // 0
    &&L_ADD,    // 1
    &&L_PRINT,  // 2
    &&L_HALT    // 3
};

// ...
goto *labels[opcode];
```

## Why It Was Removed

### Problems with C backend:

1. **Dependency on external compiler** — clang/gcc required
2. **Extra compilation step** — slower iteration
3. **Non-deterministic output** — compiler flags, optimizations varied
4. **Large binary size** — C intermediate code bloated artifacts
5. **Harder to verify** — C semantics differ from Norscode

### Advantages of bytecode VM:

- **Self-contained** — only Norscode native needed
- **Deterministic** — bytecode execution always identical
- **Reproducible** — no compiler variability
- **Lightweight** — NCB JSON is compact
- **Understandable** — Norscode semantics match source

## For Maintainers: Regen Process

The C backend is **still used internally** for one purpose: regenerating the stage-0 seed.

```bash
# Maintainer only:
REGEN=1 bash tools/build_norscode_native.sh

# This internally:
# 1. Compiles Norscode compiler itself to NCB JSON
# 2. Runs ncb_to_c.no to generate C code
# 3. Compiles C with clang to get new stage-0 binary
```

See [docs/SELVSTENDIGHET_PLAN.md](../docs/SELVSTENDIGHET_PLAN.md) for details.

## Migration Guide

If you have code that relied on C backend:

**Old (Fase 1-2):**
```bash
nc bygg app.no                  # Generated C code
gcc -O2 app.c -o app            # Compiled to native
./app                           # Ran binary
```

**New (Fase 3):**
```bash
nc run app.no                   # Bytecode execution (fast enough)
```

**Future (Fase 6):**
```bash
nc bygg --mål elf64 app.no      # Direct ELF generation
./app.elf                       # Native binary
```

## Code Location

All C generation code is preserved in Git history:

```bash
git log --all --full-history -- "selfhost/ncb_to_c.no"
git log --all --full-history -- "selfhost/gen_dispatch.no"
```

For understanding how it worked, check commits before Fase 3 (June 2026).

## See Also

- [docs/BYTECODE_VM_PRIMARY_PATH.md](../docs/BYTECODE_VM_PRIMARY_PATH.md) — Current approach
- [docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md](../docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md) — What was removed
- [selfhost/ncb_to_c.no](../selfhost/ncb_to_c.no) — Implementation (still in repo for regen)
