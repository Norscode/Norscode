# Norscode Native-First Enforcement

Status: **FASE 3 — Active enforcement**

## Policy

From June 2026 onward, Norscode development follows **native-first** principle:

- **Normal vei:** `bin/nc` → `dist/norscode_native` (mandatory)
- **Bootstrap:** `bin/bootstrap` (only for stage-0 seed generation)
- **Legacy / maintainer:** C bootstrap tools (`archive/legacy_c_backend/`) — eksplisitt maintainer-bru for seed-fornying, ikkje dagleg workflow

### What "native-first" means

1. **`./bin/nc` is the single entry point** for all user commands
   - `nc run`, `nc test`, `nc compile`, `nc check`, `nc build` all require `dist/norscode_native`
   - No fallback to Python, C VMs, or alternative paths
   - Fails with clear error if binary missing

2. **`dist/norscode_native` is the stage-0 runtime**
   - Either from `bootstrap/stage0/` (seed)
   - Or built from `tools/build_norscode_native.sh`
   - Maintainer-only exception: regenerated via `tools/maint/regen_native.sh` til isolert maintainer-output under `build/maintainer_regen/`
   - Maintainer-brua er verifisert via `tools/verify_nc_main_host.sh`, `tools/maint/regen_verify.sh` og `tools/maint/verify_l6.sh`
   - Normalflata er verifisert via `tools/verify_selvstendighet.sh`, inkludert `tests/test_json.no` og `tests/test_json_invalid.no`

3. **No hidden fallback chains**
   - Removed: Python orbit in normal compile pipeline
   - Removed: C bytecode VM as primary executor
   - Removed: Implicit bootstrap flipping

### Verification rules

**Gate: All commands MUST use native**

```bash
# REQUIRED: dist/norscode_native exists and is executable
test -x dist/norscode_native

# FAIL: If any command falls back to bin/bootstrap, legacy tools/c_minimal_vm/,
# or snapshot oracles (except for explicit --snapshot-compare)
```

**CI enforcement:**

CI bruker `bash tools/no_c_python_active_surface.sh` for aa stoppe nye Python-/C-spor i aktiv flyt.

**README and documentation** list native-first first, og omtaler generated-C som maintainer-bru, ikkje som normalveg.

**Current verification state:** både maintainer-løypa og den vanlege `./bin/nc`-løypa er grøne.

## Examples

### ✅ Native-first (correct)

```bash
./bin/nc run app.no
./bin/nc test
./bin/nc compile program.no
./bin/nc check program.no
./bin/nc build program.no out.ncb.json
```

All directly use `dist/norscode_native` via environment dispatch.

### ❌ Fallback (incorrect, removed)

```bash
# These should NOT be part of normal workflow:
./bin/bootstrap run app.no
nc tools/c_minimal_vm run program.ncb.json
```

## Migration path (Fase 3)

1. ✅ Verify `bin/nc` always uses native (done)
2. ✅ Remove Python-based compile from normal scripts
3. ✅ Add CI gate that fails if legacy fallback detected
4. ✅ Update README to show native-first path first
5. ✅ Move old bootstrap docs to archive/

## See also

- [docs/SELFHOST_HANDLINGSPLAN.md](SELFHOST_HANDLINGSPLAN.md) — Fase 3 plan
- [docs/SELVSTENDIGHET_PLAN.md](SELVSTENDIGHET_PLAN.md) — L1–L6 status
- [archive/](../archive/) — Historical bootstrap and C VM documentation
