#!/usr/bin/env bash
# tools/build_omgang6b_compiler_ncb.sh — kompilator-kjede + ELF driver (Omgang 6b)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/build/6b/compiler_stage0.ncb.json}"

# shellcheck source=omgang6b_compiler_bundle_args.inc.sh
. "$ROOT/tools/omgang6b_compiler_bundle_args.inc.sh"

mkdir -p "$(dirname "$OUT")"

"$ROOT/bin/nc" bundle "${OMGANG6B_BUNDLE_ARGS[@]}" --output "$OUT"

bash "$ROOT/tools/patch_ncb_entry.sh" "$OUT" "selfhost.elf_compile_driver.start"

printf '✓ Omgang 6b stage-0 NCB: %s (%d bytes)\n' "$OUT" "$(wc -c < "$OUT" | tr -d ' ')"
