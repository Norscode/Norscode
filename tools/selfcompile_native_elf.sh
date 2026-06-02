#!/usr/bin/env bash
# tools/selfcompile_native_elf.sh — Omgang 6: A→B ELF sjølvkompilering (byte-paritet)
#
# Gen1 og Gen2: same stage-0 byggjer same .no → ELF to gonger via bygg-native.
# Bestått når output er identisk (deterministisk native codegen).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

SRC="$ROOT/tests/fixtures/omgang6_selfcompile.no"
V1="$ROOT/build/omgang6/elf_v1"
V2="$ROOT/build/omgang6/elf_v2"

mkdir -p "$ROOT/build/omgang6"

if [ ! -f "$SRC" ]; then
    printf 'Feil: manglar %s\n' "$SRC" >&2
    exit 1
fi

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

printf '=== Omgang 6 ELF sjølvkompilering ===\n\n'

printf '[1/3] Gen1: bygg-native...\n'
"$ROOT/bin/nc" bygg-native "$SRC" "$V1"
B1="$(wc -c < "$V1" | tr -d ' ')"
printf '  [OK] %s bytes\n\n' "$B1"

printf '[2/3] Gen2: bygg-native (same kilde)...\n'
"$ROOT/bin/nc" bygg-native "$SRC" "$V2"
B2="$(wc -c < "$V2" | tr -d ' ')"
printf '  [OK] %s bytes\n\n' "$B2"

printf '[3/3] Byte-paritet Gen1 == Gen2...\n'
if cmp -s "$V1" "$V2"; then
    printf '  [OK] %s bytes identiske\n\n' "$B1"
    printf '=== Omgang 6 ELF sjølvkompilering: BESTÅTT ===\n'
    exit 0
fi

printf '  [FEIL] ELF differ (v1=%s, v2=%s bytes)\n' "$B1" "$B2" >&2
cmp -l "$V1" "$V2" 2>/dev/null | head -5 || true
exit 1
