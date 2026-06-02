#!/usr/bin/env bash
# tools/selfcompile_l5b_mini.sh — L5b smoke: liten bundle, VM Gen2 (raskare enn full kompilator)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
NC="$ROOT/bin/nc"
NATIVE="$ROOT/dist/norscode_native"

BUNDLE_ARGS=(
    selfhost.json=selfhost/json.no
    selfhost.kompiler=selfhost/kompiler.no
    selfhost.vm=selfhost/vm.no
    selfhost.bundler=selfhost/bundler.no
)
BUNDLE_ARGS_STR="${BUNDLE_ARGS[*]}"

V1="build/l5b/mini_v1.ncb.json"
V2="build/l5b/mini_v2.ncb.json"

mkdir -p build/l5b

printf '=== L5b-mini (VM byte-paritet, liten bundle) ===\n\n'
"$NC" bundle "${BUNDLE_ARGS[@]}" --output "$V1"
env NORSCODE_CMD=l5b-gen2 \
    NORSCODE_L5B_V1="$V1" \
    NORSCODE_L5B_V2="$V2" \
    NORSCODE_BUNDLE_ARGS="$BUNDLE_ARGS_STR" \
    "$NATIVE"
cmp -s "$V1" "$V2"
printf '\n=== L5b-mini: BESTÅTT ===\n'
