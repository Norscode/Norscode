#!/usr/bin/env bash
# tools/verify_selvstendighet.sh — verifiser L1–L6 selvstendighet (utan Python)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Norscode selvstendighet (L1–L6) ===\n\n'

printf '0. Python-gate i tools/...\n'
bash "$ROOT/tools/python_dependency_audit.sh"
printf '\n'

printf '0b. Ingen legacy C-VM under tools/...\n'
bash "$ROOT/tools/no_legacy_cvm.sh"
printf '\n'

printf '1. L6: ingen committed bootstrap/c/*.c...\n'
tracked="$(git -C "$ROOT" ls-files 'bootstrap/c/*.c' 2>/dev/null || true)"
if [ -n "$tracked" ]; then
    printf '  [FEIL] Committed C i bootstrap/c/:\n%s\n' "$tracked" >&2
    exit 1
fi
printf '  [OK]\n\n'

printf '2. Stage-0: norscode_native...\n'
bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

printf '3. Selfhost bootstrap-gate (steg A+B)...\n'
./bin/nc selfhost-bootstrap-gate
printf '\n'

printf '4. Bootstrap-self (steg C)...\n'
./bin/nc bootstrap-self
printf '\n'

printf '5. L5 sjølvkompilering (Gen1 == Gen2)...\n'
./bin/nc selfcompile-l5
printf '\n'

printf '6. L5b Gen1-bytekode → Gen2 (bygg_bundle i NCB)...\n'
./bin/nc selfcompile-l5b
printf '\n'

printf '7. Testsuite (native)...\n'
./bin/nc test
printf '\n'

printf '=== Selvstendighet L1–L6: BESTÅTT ===\n'
