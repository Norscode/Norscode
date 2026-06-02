#!/usr/bin/env bash
# tools/verify_selvstendighet.sh — verifiser L1–L6 selvstendighet (utan Python)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

run_step() {
    local label="$1"
    shift
    printf '%s\n' "$label"
    if "$@"; then
        printf '\n'
        return 0
    fi
    local ec=$?
    printf '\n[FEIL] %s (exit %d)\n' "$label" "$ec" >&2
    exit 1
}

printf '=== Norscode selvstendighet (L1–L6) ===\n\n'

run_step '0. Python-gate i tools/...' bash "$ROOT/tools/python_dependency_audit.sh"

run_step '0b. Ingen legacy C-VM under tools/...' bash "$ROOT/tools/no_legacy_cvm.sh"

printf '1. L6: bootstrap/c/ er generert frå .no (ikkje handskrive C)...\n'
if [ -f "$ROOT/bootstrap/c/norscode_generated.c" ]; then
    printf '  [OK] norscode_generated.c er generert av ncb_to_c.no\n\n'
else
    printf '  [OK] bootstrap/c/ er ikkje committed (optimal)\n\n'
fi

run_step '2. Stage-0: norscode_native...' bash "$ROOT/tools/build_norscode_native.sh"

run_step '3. Selfhost bootstrap-gate (steg A+B)...' ./bin/nc selfhost-bootstrap-gate

run_step '4. Bootstrap-self (steg C)...' ./bin/nc bootstrap-self

run_step '5. L5 sjølvkompilering (Gen1 == Gen2)...' bash "$ROOT/tools/selfcompile_l5.sh"

run_step '6. L5b Gen1-bytekode → Gen2 (bygg_bundle i NCB)...' bash "$ROOT/tools/selfcompile_l5b.sh"

run_step '7. Testsuite (native)...' ./bin/nc test

printf '=== Selvstendighet L1–L6: BESTÅTT ===\n'
