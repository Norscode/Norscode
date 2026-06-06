#!/usr/bin/env bash
# tools/maint/verify_l6.sh — L6: seed → regen → clang (lokal verifikasjon)
#
# bootstrap/maint/c/*.c er generert frå .no; committed kopi er tillatt for CI-bootstrap.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

printf '=== L6: seed → regen → clang ===\n\n'

printf '1. Sjekk at bootstrap/maint/c/*.c er generert (ikkje handskrive)...\n'
if [ -f "$ROOT/bootstrap/maint/c/norscode_generated.c" ]; then
    printf '  [OK] norscode_generated.c er generert av ncb_to_c.no\n\n'
else
    printf '  [OK] bootstrap/maint/c/ finst ikkje (vert generert ved REGEN=1)\n\n'
fi

printf '2. Bygg frå seed + regen (slettar generert c om det finst)...\n'
rm -f "$ROOT/bootstrap/maint/c/norscode_generated.c" "$ROOT/bootstrap/maint/c/nc_dispatch.c"
rm -f "$ROOT/dist/norscode_native"
bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

printf '3. Deterministisk regen (to kjøringar)...\n'
bash "$ROOT/tools/maint/regen_verify.sh"
printf '\n'

printf '=== L6: BESTÅTT ===\n'
