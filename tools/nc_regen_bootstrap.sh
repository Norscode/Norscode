#!/bin/sh
# tools/nc_regen_bootstrap.sh — regenerer bootstrap-NCBs utan Python
#
# MERK: bootstrap/kompiler.ncb.json krev Python-kompilert NCB som startpunkt.
# Sjølvhostings-bug: selfhost-kompilatoren produserer avkorta bytecode for
# store .no-filer (t.d. ir_to_bytecode.no). Bruk Python éin gong for å laga
# startpunktet, deretter kan stdlib-NCBs regenererast utan Python.
#
# BRUK:
#   sh tools/nc_regen_bootstrap.sh         # regenerer stdlib-NCBs
#   sh tools/nc_regen_bootstrap.sh --full  # full bundle (krev korrekt starter-NCB)

set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
NC_VM="${NC_VM:-./dist/nc-vm}"

if [ ! -x "$NC_VM" ]; then
    printf '✗ dist/nc-vm ikkje funnen. Køyr: sh tools/bootstrap.sh\n' >&2; exit 1
fi

if [ "${1:-}" = "--full" ]; then
    printf 'Genererer full bootstrap/kompiler.ncb.json via --nc-bundle...\n'
    TMP="$(mktemp /tmp/nc_bundle_XXXXXX.ncb.json 2>/dev/null || echo /tmp/nc_bundle_$$.ncb.json)"
    "$NC_VM" --nc-bundle \
        selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no \
        selfhost.parser=selfhost/parser.no \
        selfhost.compiler.semantic=selfhost/compiler/semantic.no \
        selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no \
        selfhost.kompiler=selfhost/kompiler.no \
        --output "$TMP"
    cp "$TMP" bootstrap/kompiler.ncb.json
    rm -f "$TMP"
    printf '✓ bootstrap/kompiler.ncb.json: %d bytes\n' "$(wc -c < bootstrap/kompiler.ncb.json)"
fi

printf 'Oppdaterer stdlib-NCBs (Python-fri)...\n'
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
    "$NC_VM" --nc-compile "$_mod" "$_outname" >/dev/null 2>&1 && _ok=$((_ok+1))
done
printf '✓ %d stdlib-NCBs oppdatert\n' "$_ok"

printf 'Røyktesting...\n'
printf 'funksjon start() { skriv("regen OK") }' > /tmp/_nc_regen_smoke.no
_res=$("$NC_VM" --nc-run /tmp/_nc_regen_smoke.no 2>/dev/null)
if [ "$_res" = "regen OK" ]; then
    printf '✓ Røyktest bestått\n\n✓ Bootstrap regenerert!\n'
else
    printf '✗ Røyktest feilet: %s\n' "$_res" >&2; exit 1
fi
