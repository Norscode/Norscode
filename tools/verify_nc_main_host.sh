#!/usr/bin/env bash
# tools/verify_nc_main_host.sh — maintainer-smoke for opt-in .no-host / nc_main
#
# Dette er ikkje normal dagleg flyt. Skriptet verifiserer den opt-in host-banen
# som krev maintainer-regenerering for aa bygge bundle med nc_main.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
NC="$ROOT/dist/norscode_native"
VERIFY_ROOT="$ROOT/build/verify_nc_main_host"
NCB="$VERIFY_ROOT/kompiler.ncb.json"

printf '=== nc_main host-verifikasjon (maintainer / opt-in) ===\n\n'

rm -rf "$VERIFY_ROOT"
mkdir -p "$VERIFY_ROOT"

printf '1. Regenererer og byggjer i isolert maintainer-modus (inkl. nc_main i bundle)...\n'
BOOTSTRAP_C_ROOT="$VERIFY_ROOT" NORSCODE_BOOTSTRAP_C=1 REGEN=1 bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

if ! grep -q 'selfhost\.nc_main\.start' "$NCB"; then
    printf 'Feil: %s manglar selfhost.nc_main.start etter isolert regen\n' "$NCB" >&2
    exit 1
fi

printf '2. selftest via nc_main...\n'
NORSCODE_CMD=selftest "$NC"
printf '\n'

printf '3. Enkel run via nc_main...\n'
NORSCODE_CMD=run NORSCODE_FILE=tests/test_nc_main_smoke.no "$NC" >/dev/null
printf '  ✓ tests/test_nc_main_smoke.no\n\n'

printf '4. Math-smoke via nc_main (bruk std.math)...\n'
_out="$(NORSCODE_CMD=run NORSCODE_FILE=tests/test_math.no "$NC" 2>&1)"
printf '%s' "$_out" | grep -q 'test_math OK' || {
    printf 'Feil: forventa "test_math OK", fekk:\n%s\n' "$_out" >&2
    exit 1
}
printf '  ✓ tests/test_math.no\n\n'

printf '=== nc_main host-verifikasjon (maintainer / opt-in): BESTÅTT ===\n'
