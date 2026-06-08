#!/usr/bin/env bash
# tools/maint/regen_native.sh — regenerer bootstrap/maint/c/ frå Norscode (utan Python)
#
# Krever: dist/norscode_native (seed), clang, selfhost/*.no
#
# Steg:
#   1. bundle → bootstrap/kompiler.ncb.json
#   2. archive/legacy_c_backend/ncb_to_c.no → bootstrap/maint/c/norscode_generated.c
#   3. selfhost/maint/gen_dispatch.no → bootstrap/maint/c/nc_dispatch.c
#   4. clang → dist/norscode_native (valfritt med --rebuild)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
NC="$ROOT/bin/nc"
REBUILD=0
# REGEN_ROOT: der output lagrast (standard bootstrap/). Bruk build/regen_verify for sjekk utan å endre repo.
REGEN_ROOT="${REGEN_ROOT:-$ROOT/bootstrap}"

if [ "${1:-}" = "--rebuild" ]; then
    REBUILD=1
fi

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    printf '✗ Trenger dist/norscode_native (seed). Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

printf '=== Regenerer bootstrap/maint/c/ (utan Python) ===\n\n'

printf '[1/4] Bundle kompilator-modular...\n'
TMP="$(mktemp "${TMPDIR:-/tmp}/nc_bundle_XXXXXX")"
trap 'rm -f "$TMP"' EXIT
# Same modular sett som L5 (runtime-kompilator); maint/ncb_to_c og maint/gen_dispatch køyrast som eigne filer.
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

printf '[3/4] gen_dispatch → %s/maint/c/nc_dispatch.c...\n' "$REGEN_ROOT"
env NORSCODE_CMD=run \
    NORSCODE_FILE="$ROOT/selfhost/maint/gen_dispatch.no" \
    NC_NCB_INPUT="$REGEN_ROOT/kompiler.ncb.json" \
    NC_DISPATCH_OUTPUT="$REGEN_ROOT/maint/c/nc_dispatch.c" \
    "$ROOT/dist/norscode_native"
printf '  ✓ nc_dispatch.c (%d bytes)\n' "$(wc -c < "$REGEN_ROOT/maint/c/nc_dispatch.c" | tr -d ' ')"

if [ "$REBUILD" -eq 1 ]; then
    printf '[4/4] Bygg ny dist/norscode_native...\n'
    rm -f "$ROOT/dist/norscode_native"
    bash "$ROOT/tools/build_norscode_native.sh"
    printf '  ✓ dist/norscode_native oppdatert\n'
else
    printf '[4/4] Hopp over clang (--rebuild for å byggje ny binær)\n'
fi

printf '\n=== Regen ferdig ===\n'
printf 'Køyr: bash tools/verify_selvstendighet.sh\n'
printf 'L6: bootstrap/maint/c/ og kompiler.ncb.json er generert lokalt (ikkje commit).\n'
