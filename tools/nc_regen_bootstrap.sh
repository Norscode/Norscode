#!/bin/sh
# tools/nc_regen_bootstrap.sh — regenerer bootstrap-NCBs
#
# Brukar bin/nc (norscode_native) for alle operasjonar.
# nc-vm trengst berre som fallback om norscode_native ikkje finst.
#
# BRUK:
#   sh tools/nc_regen_bootstrap.sh         # regenerer stdlib-NCBs
#   sh tools/nc_regen_bootstrap.sh --full  # full bundle + norscode_native

set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
NC="$ROOT/bin/nc"
NC_VM="${NC_VM:-$ROOT/dist/nc-vm}"

# Sjekk at bin/nc eller nc-vm finst
if [ ! -x "$ROOT/dist/norscode_native" ] && [ ! -x "$NC_VM" ]; then
    printf '✗ Trenger dist/norscode_native eller dist/nc-vm.\n' >&2
    printf '  Køyr: bash tools/build-bootstrap-binary.sh\n' >&2
    exit 1
fi

if [ "${1:-}" = "--full" ]; then
    printf 'Genererer full bootstrap/kompiler.ncb.json...\n'
    TMP="$(mktemp /tmp/nc_bundle_XXXXXX.ncb.json 2>/dev/null || echo /tmp/nc_bundle_$$.ncb.json)"
    "$NC" bundle \
        selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no \
        selfhost.parser=selfhost/parser.no \
        selfhost.compiler.semantic=selfhost/compiler/semantic.no \
        selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no \
        selfhost.kompiler=selfhost/kompiler.no \
        selfhost.json=selfhost/json.no \
        selfhost.vm=selfhost/vm.no \
        selfhost.main=selfhost/main.no \
        selfhost.bundler=selfhost/bundler.no \
        --output "$TMP"
    cp "$TMP" bootstrap/kompiler.ncb.json
    rm -f "$TMP"
    printf '✓ bootstrap/kompiler.ncb.json: %d bytes\n' "$(wc -c < bootstrap/kompiler.ncb.json)"

    # Regenerer C-filer og norscode_native
    printf 'Regenererer bootstrap/c/ og norscode_native...\n'
    NC_NCB_INPUT="$ROOT/bootstrap/kompiler.ncb.json" \
    NC_C_OUTPUT="$ROOT/bootstrap/c/norscode_generated.c" \
        "$NC" run "$ROOT/selfhost/ncb_to_c.no"
    python3 "$ROOT/tools/gen_dispatch.py" "$ROOT/bootstrap/kompiler.ncb.json" \
        > "$ROOT/bootstrap/c/nc_dispatch.c"
    bash "$ROOT/tools/build_norscode_native.sh"
    printf '✓ norscode_native regenerert\n'
fi

printf 'Oppdaterer stdlib-NCBs...\n'
mkdir -p bootstrap/stdlib
_ok=0
for _mod in \
    std/math.no std/tekst.no std/liste.no std/ordbok.no std/json.no \
    std/feil.no std/env.no std/io.no std/fil.no std/log.no std/path.no \
    std/cache.no \
    selfhost/common.no selfhost/ir_contract.no selfhost/compiler.no \
    selfhost/compiler_bridge.no; do
    [ -f "$_mod" ] || continue
    _outname="bootstrap/stdlib/$(echo "$_mod" | sed 's|/|_|g' | sed 's|\.no$|.ncb.json|')"
    "$NC" compile "$_mod" "$_outname" >/dev/null 2>&1 && _ok=$((_ok+1))
done
printf '✓ %d stdlib-NCBs oppdatert\n' "$_ok"

printf 'Røyktesting...\n'
printf 'funksjon start() { skriv("regen OK") }' > /tmp/_nc_regen_smoke.no
_res=$("$NC" run /tmp/_nc_regen_smoke.no 2>/dev/null)
if [ "$_res" = "regen OK" ]; then
    printf '✓ Røyktest bestått\n\n✓ Bootstrap regenerert!\n'
else
    printf '✗ Røyktest feilet: %s\n' "$_res" >&2; exit 1
fi
