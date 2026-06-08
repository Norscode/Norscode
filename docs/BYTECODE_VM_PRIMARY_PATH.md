# Bytecode VM as Primary Execution Path

Status: **Fase 3 (June 2026)** — C backend removed from normal workflow

## Architecture

Normal Norscode compilation pipeline (current):

```
source.no
    ↓
dist/norscode_native (lexer, parser, semantic)
    ↓
NCB JSON (bytecode intermediate format)
    ↓
selfhost/vm.no (executes bytecode)
    ↓
Program output
```

**No C compilation step.** No clang, gcc, or other C compiler needed.

### What happened to C backend?

| Phase | Status | Path |
|-------|--------|------|
| **Fase 1–2** | Historical / maintenance | `archive/legacy_c_backend/ncb_to_c.no` → C code → clang (vedlikehald/sjekkbanar) |
| **Fase 3 (now)** | Deprecation (maintainer lane) | Vedlikehaldsregen går via `tools/maint/regen_native.sh` → `bootstrap/maint/c/` og er ikkje del av normal bruk |
| **Fase 6+ (planned)** | Replaced | Native ELF emitter in `.no` (no C needed ever) |

## Normal User Commands

All of these work **without any C toolchain**:

```bash
./bin/nc run app.no                  # Compile → NCB → VM
./bin/nc test                        # All tests via VM
./bin/nc compile program.no out.json # Bytecode only
./bin/nc check program.no            # Semantic check only
./bin/nc build program.no out.json   # Build bytecode
```

## What Requires C (Developer-only)

Only explicit maintenance tasks in `tools/maint/` need C toolchain:

```bash
# Bootstrap L6 only (maintainer lane; regenerate stage-0 bridge when needed):
NORSCODE_BOOTSTRAP_C=1 bash tools/build_norscode_native.sh

# Maintainer: regen bootstrap/maint/c/ from Norscode
bash tools/maint/regen_native.sh
```

These are **not** part of normal workflow. See [SELVSTENDIGHET_PLAN.md](SELVSTENDIGHET_PLAN.md).

## Bytecode Format: NCB JSON

Standard intermediate format (language-independent):

```json
{
  "version": "1.0",
  "instructions": [
    {"opcode": "PUSH", "arg": 42},
    {"opcode": "PUSH", "arg": 7},
    {"opcode": "ADD"},
    {"opcode": "PRINT"},
    {"opcode": "HALT"}
  ]
}
```

Executed by `selfhost/vm.no` with deterministic semantics.

**Advantages:**
- Language-independent (could be compiled by other languages)
- No C compilation step
- Reproducible execution
- Single canonical VM implementation

## For Native Binary Output (Fase 6+)

Currently: NCB JSON → `nc bygg --mål elf64` (via native codegen)

Planned (Fase 6+): Direct ELF emission in `.no` without C.

```
source.no → NCB JSON → ELF (via native_codegen.no)
                       ↓
                    a.out (executable)
```

No clang needed.

## Implications

### ✅ What this enables

- **Minimal dependencies** — only dist/norscode_native required
- **Reproducible builds** — bytecode is deterministic
- **Fast iteration** — no C compilation bottleneck
- **Cross-platform** — same VM everywhere
- **Language agnostic** — other languages could emit NCB JSON

### ❌ What this prevents

- Direct C interop (for now)
- Inline assembly
- C library linking

**Future:** Planned for Fase 7+ (FFI design in progress).

## Migration from C Backend

**If you were using C backend:**

Old path (removed):
```bash
./bin/nc bygg app.no                 # Emitted C code
gcc -O2 app.c -o app                 # Compiled C
./app                                # Ran native binary
```

New path (Fase 3):
```bash
./bin/nc compile app.no out.ncb.json # Bytecode
./bin/nc run app.no                  # VM executes
```

Future path (Fase 6):
```bash
./bin/nc bygg --mål elf64 app.no     # Direct ELF (no C)
./app.elf                            # Native binary
```

## See also

- [docs/NATIVE_FIRST_ENFORCEMENT.md](NATIVE_FIRST_ENFORCEMENT.md)
- [docs/SELVSTENDIGHET_PLAN.md](SELVSTENDIGHET_PLAN.md)
- [selfhost/vm.no](../selfhost/vm.no) — Bytecode VM implementation
- [selfhost/bytecode_format.no](../selfhost/bytecode_format.no) — Format spec
