#!/usr/bin/env bash
# tools/maint/verify_l6.sh — historisk verifikasjon av L6: seed → regen → clang
#
# Dette er ikkje normal build/test-flyt.
# bootstrap/maint/c/ er lokal historisk output frå regen-lanen.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
L6_ROOT="$ROOT/build/verify_l6"

printf '=== L6 historisk verifikasjon: seed → regen → clang ===\n\n'

printf '1. Klargjer isolert historisk output i build/verify_l6/...\n'
rm -rf "$L6_ROOT"
mkdir -p "$L6_ROOT"
printf '  [OK] brukar %s som isolert historisk output\n\n' "$L6_ROOT"

printf '2. Historisk bygg frå seed + regen (utan aa skrive til bootstrap/maint/c/)...\n'
rm -f "$ROOT/dist/norscode_native"
BOOTSTRAP_C_ROOT="$L6_ROOT" NORSCODE_BOOTSTRAP_C=1 REGEN=1 bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

printf '3. Deterministisk historisk regen (to køyringar)...\n'
bash "$ROOT/tools/maint/regen_verify.sh"
printf '\n'

printf '=== L6 historisk verifikasjon: BESTÅTT ===\n'
