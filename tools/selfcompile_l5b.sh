#!/usr/bin/env bash
# tools/selfcompile_l5b.sh — L5b: Gen1-NCB køyrer Gen2 via VM (byte-paritet)
#
# Gen1: stage-0 byggjer kompilator-bundle (bootstrap-host med C-dispatch).
# Gen2: nc_exec køyrer selfhost.bundler.bygg_bundle-bytekode frå Gen1-NCB
# (kompiler-kall går via bootstrap-host for hastigheit).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
NC="$ROOT/bin/nc"
NATIVE="$ROOT/dist/norscode_native"

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
BUNDLE_ARGS_STR="${BUNDLE_ARGS[*]}"

V1="build/l5b/compiler_v1.ncb.json"
V2="build/l5b/compiler_v2.ncb.json"

if [ ! -x "$NATIVE" ]; then
    printf '✗ Trenger dist/norscode_native. Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

mkdir -p build/l5b

printf '=== L5b: VM sjølvkompilering (byte-paritet) ===\n\n'

printf '[1/3] Gen1: kompilator-bundle (bootstrap-host)...\n'
"$NC" bundle "${BUNDLE_ARGS[@]}" --output "$V1"
if [ ! -f "$V1" ]; then
    printf '  [FEIL] Gen1 skreiv ikkje %s\n' "$V1" >&2
    exit 1
fi
V1_BYTES="$(wc -c < "$V1" | tr -d ' ')"
printf '  ✓ %s bytes\n\n' "$V1_BYTES"

printf '[2/3] Gen2: bundle via Gen1-bytekode (bygg_bundle)...\n'
_gen2_ec=0
if ! env NORSCODE_CMD=l5b-gen2 \
    NORSCODE_L5B_V1="$V1" \
    NORSCODE_L5B_V2="$V2" \
    NORSCODE_BUNDLE_ARGS="$BUNDLE_ARGS_STR" \
    "$NATIVE"; then
    _gen2_ec=$?
    printf '  [FEIL] l5b-gen2 feila med exit %d\n' "$_gen2_ec" >&2
    exit 1
fi
if [ ! -f "$V2" ]; then
    printf '  [FEIL] Gen2 skreiv ikkje %s - sjekk l5b-gen2 og host_kall_bygg_bundle\n' "$V2" >&2
    exit 1
fi
printf '\n'

printf '[3/3] Byte-paritet Gen1 == Gen2...\n'
if cmp -s "$V1" "$V2" 2>/dev/null; then
    printf '  [OK] %s bytes identiske\n\n' "$V1_BYTES"
    printf '=== L5b: BESTÅTT ===\n'
    printf 'Kompilator-bundle frå VM er byte-identisk med bootstrap-host.\n'
    exit 0
fi

V2_BYTES="$(wc -c < "$V2" | tr -d ' ')"
printf '  [FEIL] Bundles differ (v1=%s, v2=%s bytes)\n' "$V1_BYTES" "$V2_BYTES"
cmp -l "$V1" "$V2" 2>/dev/null | head -5 || true
exit 1
