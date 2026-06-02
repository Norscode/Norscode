#!/usr/bin/env bash
# tools/selfcompile_l5.sh — L5: kompilator-kompilerer-kompilator (byte-paritet)
#
# Gen1 og Gen2: same norscode_native byggjer full kompilator-bundle to gonger.
# L5 bestått når output er identisk byte for byte (deterministisk sjølvkompilering).
#
# Merk: VM-køyring av Gen1-NCB (selfhost/vm) er L5b — krev VM/dispatch-paritet.
# Denne testen verifiserer determinisme i kompilator-pipeline, som er naudsynt for L5.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
NC="$ROOT/bin/nc"

BUNDLE_ARGS=(
    selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no
    selfhost.parser=selfhost/parser.no
    selfhost.compiler.semantic=selfhost/compiler/semantic.no
    selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no
    selfhost.kompiler=selfhost/kompiler.no
    selfhost.json=selfhost/json.no
    selfhost.vm=selfhost/vm.no
    selfhost.bundler=selfhost/bundler.no
)

V1="build/l5/compiler_v1.ncb.json"
V2="build/l5/compiler_v2.ncb.json"

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    printf '✗ Trenger dist/norscode_native. Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

mkdir -p build/l5

printf '=== L5 sjølvkompilering (byte-paritet) ===\n\n'

printf '[1/3] Gen1: kompilator-bundle (første kompilasjon)...\n'
"$NC" bundle "${BUNDLE_ARGS[@]}" --output "$V1"
if [ ! -f "$V1" ]; then
    printf '  [FEIL] Gen1 skreiv ikkje %s\n' "$V1" >&2
    exit 1
fi
V1_BYTES="$(wc -c < "$V1" | tr -d ' ')"
printf '  ✓ %s bytes\n\n' "$V1_BYTES"

printf '[2/3] Gen2: kompilator-bundle (andre kompilasjon, same native)...\n'
"$NC" bundle "${BUNDLE_ARGS[@]}" --output "$V2"
if [ ! -f "$V2" ]; then
    printf '  [FEIL] Gen2 skreiv ikkje %s\n' "$V2" >&2
    exit 1
fi
V2_BYTES="$(wc -c < "$V2" | tr -d ' ')"
printf '  ✓ %s bytes\n\n' "$V2_BYTES"

printf '[3/3] Byte-paritet Gen1 == Gen2...\n'
if cmp -s "$V1" "$V2" 2>/dev/null; then
    printf '  [OK] %s bytes identiske\n\n' "$V1_BYTES"
    printf '=== L5: BESTÅTT ===\n'
    printf 'Kompilator-bundle er deterministisk (to generasjonar, same output).\n'
    exit 0
fi

printf '  [FEIL] Bundles differ (v1=%s, v2=%s bytes)\n' "$V1_BYTES" "$V2_BYTES"
cmp -l "$V1" "$V2" 2>/dev/null | head -5 || true
exit 1
