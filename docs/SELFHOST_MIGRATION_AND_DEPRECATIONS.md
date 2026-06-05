# Norscode Selfhost Migration and Deprecations

Status: **Fase 2 (June 2026)** — Native-first enforcement

## Overview

This document tracks what was **removed**, **deprecated**, and **why** during selfhost migration.

## Removed in Fase 2

### ❌ Python-based compilation pipeline

**What was removed:**
- `tools/main.py` (Python entry point)
- `tools/compile.py` (Python compiler)
- Python invocations from normal workflow

**Why:**
- Norscode is self-compiling via `dist/norscode_native`
- Python was bootstrap-only, not needed daily
- Simpler path: `.no` → `norscode_native` → bytecode

**Migration:**
```bash
# Old (removed):
python3 tools/main.py run app.no

# New:
./bin/nc run app.no
```

### ❌ C bytecode VM as primary executor

**What was deprecated:**
- `tools/c_minimal_vm/` (legacy C-based stack VM)
- Direct C VM invocations from normal pipeline

**Why:**
- `selfhost/vm.no` is canonical VM
- Bytecode standard is NCB JSON
- C VM was bootstrap artifact only

**Status:** Moved to `archive/c_minimal_vm/` for reference.

### ⚠️ `bin/bootstrap` as normal entry point

**What changed:**
- `bin/bootstrap` is still available but **not normal path**
- Explicitly marked as bootstrap-only
- Normal users use `bin/nc` exclusively

**Why:**
- `bin/nc` is single unified entry point
- Native-first eliminates intermediate launchers

## What's Kept

### ✅ `bootstrap/stage0/` seed binaries

**Why kept:**
- Necessary for chicken-and-egg bootstrap (L6)
- Each platform needs seed binary
- Generated via `tools/maint/regen_native.sh` (no Python)

### ✅ `bootstrap/maint/c/` C generation

**Why kept:**
- Bridge between NCB JSON and native ELF
- Stage-0 verification (L6 level)
- Will be replaced by ELF emitter in `.no` (Fase 6+)

### ✅ `archive/` historical code

**What's there:**
- Old bootstrap pipelines
- C VM implementation
- Early parser experiments
- Legacy bytecode formats

**Why:** Learn from history, audit trail, reference for maintainers

## For Users: No Breaking Changes

```bash
# All of these work exactly the same:
./bin/nc run app.no
./bin/nc test
./bin/nc compile program.no
./bin/nc check program.no
```

## For Contributors: Significant Changes

**No longer do this:**
```bash
python3 tools/main.py run app.no              # ❌ Removed
python3 tools/compile.py app.no               # ❌ Removed
./bin/bootstrap run app.no                    # ❌ Use bin/nc
nc tools/c_minimal_vm run prog.ncb.json       # ❌ Removed
```

**Do this instead:**
```bash
./bin/nc run app.no                           # ✅ Native
./bin/nc compile app.no out.ncb.json          # ✅ Native
./bin/nc test                                 # ✅ Native
```

## Verification

All code is in Git history:

```bash
git log --all --full-history -- "tools/main.py"
git log --all --full-history -- "archive/c_minimal_vm/"
```

## See also

- [docs/NATIVE_FIRST_ENFORCEMENT.md](NATIVE_FIRST_ENFORCEMENT.md)
- [docs/SELFHOST_HANDLINGSPLAN.md](SELFHOST_HANDLINGSPLAN.md)
- [docs/LANE_MAP.md](LANE_MAP.md)
