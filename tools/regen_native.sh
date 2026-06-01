#!/usr/bin/env bash
# tools/regen_native.sh — regenerer bootstrap/c/ frå Norscode (utan Python)
#
# Krever: dist/norscode_native (seed), clang, selfhost/*.no
#
# Steg:
#   1. bundle → bootstrap/kompiler.ncb.json
#   2. ncb_to_c.no → bootstrap/c/norscode_generated.c
#   3. gen_dispatch.no → bootstrap/c/nc_dispatch.c
#   4. clang → dist/norscode_native (valfritt med --rebuild)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
NC="$ROOT/bin/nc"
REBUILD=0

if [ "${1:-}" = "--rebuild" ]; then
    REBUILD=1
fi

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    printf '✗ Trenger dist/norscode_native (seed). Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

printf '=== Regenerer bootstrap/c/ (utan Python) ===\n\n'

printf '[1/4] Bundle kompilator-modular...\n'
TMP="$(mktemp "${TMPDIR:-/tmp}/nc_bundle_XXXXXX")"
trap 'rm -f "$TMP"' EXIT
"$NC" bundle \
    selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no \
    selfhost.parser=selfhost/parser.no \
    selfhost.compiler.semantic=selfhost/compiler/semantic.no \
    selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no \
    selfhost.kompiler=selfhost/kompiler.no \
    selfhost.json=selfhost/json.no \
    selfhost.vm=selfhost/vm.no \
    selfhost.nc_main=selfhost/nc_main.no \
    selfhost.ncb_to_c=selfhost/ncb_to_c.no \
    selfhost.gen_dispatch=selfhost/gen_dispatch.no \
    --output "$TMP"
mkdir -p bootstrap
cp "$TMP" bootstrap/kompiler.ncb.json
printf '  ✓ bootstrap/kompiler.ncb.json (%d bytes)\n' "$(wc -c < bootstrap/kompiler.ncb.json | tr -d ' ')"

printf '[2/4] ncb_to_c → bootstrap/c/norscode_generated.c...\n'
mkdir -p bootstrap/c
env NORSCODE_CMD=run \
    NORSCODE_FILE="$ROOT/selfhost/ncb_to_c.no" \
    NC_NCB_INPUT="$ROOT/bootstrap/kompiler.ncb.json" \
    NC_C_OUTPUT="$ROOT/bootstrap/c/norscode_generated.c" \
    "$ROOT/dist/norscode_native"
printf '  ✓ norscode_generated.c (%d bytes)\n' "$(wc -c < bootstrap/c/norscode_generated.c | tr -d ' ')"

printf '[3/4] gen_dispatch → bootstrap/c/nc_dispatch.c...\n'
env NORSCODE_CMD=run \
    NORSCODE_FILE="$ROOT/selfhost/gen_dispatch.no" \
    NC_NCB_INPUT="$ROOT/bootstrap/kompiler.ncb.json" \
    NC_DISPATCH_OUTPUT="$ROOT/bootstrap/c/nc_dispatch.c" \
    "$ROOT/dist/norscode_native"
printf '  ✓ nc_dispatch.c (%d bytes)\n' "$(wc -c < bootstrap/c/nc_dispatch.c | tr -d ' ')"

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
printf 'Commit: git add bootstrap/kompiler.ncb.json bootstrap/c/\n'
