#!/usr/bin/env bash
# tools/verify_selvstendighet.sh — verifiser L1–L3 selvstendighet (utan Python)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Norscode selvstendighet (L1–L3) ===\n\n'

printf '0. Python-gate i tools/...\n'
bash "$ROOT/tools/python_dependency_audit.sh"
printf '\n'

printf '1. Stage-0: norscode_native...\n'
bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

printf '2. Selfhost bootstrap-gate (steg A+B)...\n'
./bin/nc selfhost-bootstrap-gate
printf '\n'

printf '3. Bootstrap-self (steg C)...\n'
./bin/nc bootstrap-self
printf '\n'

printf '4. Testsuite (48/48)...\n'
./bin/nc test
printf '\n'

printf '=== Selvstendighet L1–L3: BESTÅTT ===\n'
