#!/usr/bin/env bash
# tools/ncb_to_elf.sh — NCB JSON → Linux ELF64 via native_codegen_v2 (Omgang 6/6b)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NCB="${1:?bruk: ncb_to_elf.sh fil.ncb.json ut.elf}"
OUT="${2:?}"

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

# Relative stiar frå repo-root (fil_les i codegen)
case "$NCB" in
    /*) NCB_REL="$NCB" ;;
    *)  NCB_REL="$NCB" ;;
esac
case "$OUT" in
    /*) OUT_ABS="$OUT" ;;
    *)  OUT_ABS="$ROOT/$OUT" ;;
esac

mkdir -p "$(dirname "$OUT_ABS")"

NC_INPUT="$NCB_REL" NC_OUTPUT="$OUT_ABS" \
    NORSCODE_CMD=run \
    NORSCODE_FILE="$ROOT/selfhost/native_execution/native_codegen_v2.no" \
    "$ROOT/dist/norscode_native"

chmod +x "$OUT_ABS" 2>/dev/null || true
printf '✓ ELF: %s (%d bytes)\n' "$OUT_ABS" "$(wc -c < "$OUT_ABS" | tr -d ' ')"
