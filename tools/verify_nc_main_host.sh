#!/usr/bin/env bash
# tools/verify_nc_main_host.sh — smoke for .no-host (nc_main.no er no standard)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
NC="$ROOT/dist/norscode_native"
DISPATCH="$ROOT/bootstrap/c/nc_dispatch.c"

printf '=== nc_main host-verifikasjon ===\n\n'

printf '1. Regenererer og byggjer (inkl. nc_main i bundle)...\n'
REGEN=1 bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

if ! grep -q 'selfhost\.nc_main\.start' "$DISPATCH"; then
    printf 'Feil: nc_dispatch manglar selfhost.nc_main.start etter regen\n' >&2
    exit 1
fi

printf '2. selftest via nc_main...\n'
NORSCODE_CMD=selftest "$NC"
printf '\n'

printf '3. Enkel run via nc_main...\n'
NORSCODE_CMD=run NORSCODE_FILE=tests/test_nc_main_smoke.no "$NC" >/dev/null
printf '  ✓ tests/test_nc_main_smoke.no\n\n'

printf '4. Expr-smoke via nc_main...\n'
_out="$(NORSCODE_CMD=run NORSCODE_FILE=tests/test_one_plus_two.no "$NC" 2>&1)"
printf '%s' "$_out" | grep -q 'PUSH 1' || {
    printf 'Feil: forventa IR frå test_one_plus_two, fekk:\n%s\n' "$_out" >&2
    exit 1
}
printf '  ✓ tests/test_one_plus_two.no\n\n'

printf '=== nc_main host-verifikasjon: BESTÅTT ===\n'
