#!/usr/bin/env bash
# tools/build_omgang6b_compiler_ncb.sh — minimal kompilator-kjede som NCB (Omgang 6b)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/build/6b/compiler_chain.ncb.json}"

mkdir -p "$(dirname "$OUT")"

"$ROOT/bin/nc" bundle \
    selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no \
    selfhost.parser=selfhost/parser.no \
    selfhost.compiler.semantic=selfhost/compiler/semantic.no \
    selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no \
    selfhost.json=selfhost/json.no \
    selfhost.kompiler=selfhost/kompiler.no \
    --output "$OUT"

printf '✓ Omgang 6b kompilator-NCB: %s (%d bytes)\n' "$OUT" "$(wc -c < "$OUT" | tr -d ' ')"
