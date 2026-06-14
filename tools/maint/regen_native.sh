#!/usr/bin/env bash
# tools/maint/regen_native.sh — regenerer isolert maintainer-output frå Norscode (utan Python)
#
# Krever: dist/norscode_native (seed), clang, selfhost/*.no
#
# Steg:
#   1. bundle → <REGEN_ROOT>/kompiler.ncb.json
#   2. archive/legacy_c_backend/ncb_to_c.no → <REGEN_ROOT>/maint/c/norscode_generated.c
#   3. clang → dist/norscode_native (valfritt med --rebuild)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
NC="$ROOT/bin/nc"
REBUILD=0
# REGEN_ROOT: der output lagrast. Standard er isolert build-root for å unngå
# å gjere bootstrap/maint/c/ til aktiv arbeidsflate.
REGEN_ROOT="${REGEN_ROOT:-$ROOT/build/maintainer_regen}"

if [ "${1:-}" = "--rebuild" ]; then
    REBUILD=1
fi

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    printf '✗ Trenger dist/norscode_native (seed). Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

printf '=== Regenerer isolert maintainer-output (utan Python) ===\n\n'
printf 'Merk: dette er vedlikehaldsmodus for seed-fornying, ikkje normal utviklingsflyt.\n\n'

printf '[1/4] Bundle kompilator-modular...\n'
TMP="$(mktemp "${TMPDIR:-/tmp}/nc_bundle_XXXXXX")"
trap 'rm -f "$TMP"' EXIT
# Same modular sett som L5 (runtime-kompilator); C-host-tabellen blir no generert direkte i ncb_to_c-løypa.
"$NC" bundle \
    selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no \
    selfhost.parser=selfhost/parser.no \
    selfhost.compiler.semantic=selfhost/compiler/semantic.no \
    selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no \
    selfhost.kompiler=selfhost/kompiler.no \
    selfhost.json=selfhost/json.no \
    selfhost.vm=selfhost/vm.no \
    selfhost.bundler=selfhost/bundler.no \
    selfhost.nc_main=selfhost/nc_main.no \
    --output "$TMP"
mkdir -p "$REGEN_ROOT"
cp "$TMP" "$REGEN_ROOT/kompiler.ncb.json"
printf '  ✓ %s/kompiler.ncb.json (%d bytes)\n' "$REGEN_ROOT" "$(wc -c < "$REGEN_ROOT/kompiler.ncb.json" | tr -d ' ')"

printf '[2/4] ncb_to_c → %s/maint/c/norscode_generated.c...\n' "$REGEN_ROOT"
mkdir -p "$REGEN_ROOT/maint/c"
env NORSCODE_CMD=run \
    NORSCODE_FILE="$ROOT/archive/legacy_c_backend/ncb_to_c.no" \
    NC_NCB_INPUT="$REGEN_ROOT/kompiler.ncb.json" \
    NC_C_OUTPUT="$REGEN_ROOT/maint/c/norscode_generated.c" \
    "$ROOT/dist/norscode_native"
printf '  ✓ norscode_generated.c (%d bytes)\n' "$(wc -c < "$REGEN_ROOT/maint/c/norscode_generated.c" | tr -d ' ')"

if [ "$REBUILD" -eq 1 ]; then
    printf '[3/3] Bygg ny dist/norscode_native frå %s/maint/c...\n' "$REGEN_ROOT"
    rm -f "$ROOT/dist/norscode_native"
    BOOTSTRAP_C_ROOT="$REGEN_ROOT" NORSCODE_BOOTSTRAP_C=1 REGEN=1 bash "$ROOT/tools/build_norscode_native.sh"
    printf '  ✓ dist/norscode_native oppdatert frå %s/maint/c\n' "$REGEN_ROOT"
else
    printf '[3/3] Hopp over clang (--rebuild for å byggje ny binær)\n'
fi

printf '\n=== Regen ferdig ===\n'
printf 'Køyr: bash tools/verify_selvstendighet.sh\n'
printf 'L6: maintainer-output og kompiler.ncb.json er generert lokalt i %s (ikkje commit).\n' "$REGEN_ROOT"
