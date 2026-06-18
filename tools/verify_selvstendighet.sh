#!/usr/bin/env bash
# tools/verify_selvstendighet.sh — verifiser normal L1–L6-sjølvstendighet utan C/Python
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

run_step() {
    local label="$1"
    shift
    printf '%s\n' "$label"
    set +e
    "$@"
    local ec=$?
    set -e
    if [ "$ec" -eq 0 ]; then
        printf '\n'
        return 0
    fi
    printf '\n[FEIL] %s (exit %d)\n' "$label" "$ec" >&2
    if [ "$ec" -eq 2 ]; then
        printf '  Hint: exit 2 = oftast manglande fil ved cmp, eller bash-syntaxfeil i eit script.\n' >&2
    fi
    exit 1
}

printf '=== Norscode sjølvstendighet (normalflate, L1–L6) ===\n\n'

run_step '0. Python-gate i tools/...' bash "$ROOT/tools/python_dependency_audit.sh"

run_step '0b. Ingen aktiv C/Python-flate...' bash "$ROOT/tools/no_c_python_active_surface.sh"

run_step '0c. Ingen legacy C-VM under tools/...' bash "$ROOT/tools/no_legacy_cvm.sh"

printf '1. Aktiv tools-flate har ingen C/Python-kjelder...\n'
if find "$ROOT/tools" -type f \( -name '*.c' -o -name '*.h' -o -name '*.py' \) | grep . >/dev/null 2>&1; then
    printf '  [FEIL] C/Python funne under tools/\n' >&2
    exit 1
else
    printf '  [OK] tools/ er C/Python-fri\n\n'
fi

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    printf '  [FEIL] dist/norscode_native manglar. Normalflate skal ikkje bygge stage-0 her.\n' >&2
    printf '         Materialiser frå seed/release med: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

printf '2. Stage-0: dist/norscode_native finst alt (ingen rebuild i normalflate)\n\n'

run_step '3. Selfhost bootstrap-gate (steg A+B)...' ./bin/nc selfhost-bootstrap-gate

run_step '4. Bootstrap-self (steg C)...' ./bin/nc bootstrap-self

run_step '5. L5 sjølvkompilering (Gen1 == Gen2)...' bash "$ROOT/tools/selfcompile_l5.sh"

printf '6. L5b-smoke er djupare paritet og ikkje del av kort normal verifisering.\n'
printf '   Køyr ved behov: bash tools/selfcompile_l5b_mini.sh\n\n'

printf '7. Testsuite: full testflate kan køyrast separat.\n'
printf '   Køyr ved behov: ./bin/nc test\n\n'

printf '=== Sjølvstendighet L1–L6 (normalflate): BESTÅTT ===\n'
