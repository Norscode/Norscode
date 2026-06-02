#!/usr/bin/env bash
# tools/verify_l6.sh — L6: ingen committed bootstrap/c, seed → regen → bygg
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

printf '=== L6: seed → regen → clang ===\n\n'

printf '1. Sjekk at bootstrap/c/*.c ikkje er sporet av git...\n'
tracked="$(git -C "$ROOT" ls-files 'bootstrap/c/*.c' 2>/dev/null || true)"
if [ -n "$tracked" ]; then
    printf '  [FEIL] Committed C i bootstrap/c/:\n%s\n' "$tracked" >&2
    exit 1
fi
printf '  [OK] ingen committed .c i bootstrap/c/\n\n'

printf '2. Bygg frå seed + regen (slettar generert c om det finst)...\n'
rm -f "$ROOT/bootstrap/c/norscode_generated.c" "$ROOT/bootstrap/c/nc_dispatch.c"
rm -f "$ROOT/dist/norscode_native"
bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

printf '3. Deterministisk regen (to kjøringar)...\n'
bash "$ROOT/tools/regen_verify.sh"
printf '\n'

printf '=== L6: BESTÅTT ===\n'
