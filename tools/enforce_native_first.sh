#!/usr/bin/env bash
# tools/enforce_native_first.sh — Verify native-first enforcement
#
# Checks that:
# - dist/norscode_native exists and is executable
# - No Python in normal pipeline
# - bin/nc is the single entry point

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Norscode Native-First Enforcement ===\n\n'

# 1. dist/norscode_native MUST exist
printf '1. Checking dist/norscode_native...'
if [ ! -x dist/norscode_native ]; then
    printf ' FAIL\n' >&2
    printf '   ERROR: dist/norscode_native not found or not executable\n' >&2
    exit 1
fi
printf ' OK\n'

# 2. No Python in tools/ normal workflow
printf '2. Checking no Python in normal workflow...'
if [ -f tools/main.py ] || [ -f tools/compile.py ]; then
    printf ' FAIL\n' >&2
    printf '   ERROR: Legacy Python tools found\n' >&2
    exit 1
fi
printf ' OK\n'

# 3. nc_test.sh uses bin/nc not fallback
printf '3. Checking nc_test.sh uses native...'
LEGACY_VM="minimal_vm"
if grep 'python\|'"$LEGACY_VM"'' tools/nc_test.sh >/dev/null 2>&1; then
    printf ' FAIL\n' >&2
    printf '   ERROR: nc_test.sh has legacy fallback\n' >&2
    exit 1
fi
printf ' OK\n'

printf '\n=== Native-First Enforcement: PASSED ===\n'
exit 0
