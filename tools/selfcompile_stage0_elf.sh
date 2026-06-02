#!/usr/bin/env bash
# tools/selfcompile_stage0_elf.sh — Omgang 6b.3: Gen1 stage-0 ELF → Gen2 ELF (byte-paritet)
#
# Gen1: host byggjer stage-0 NCB + ELF.
# Gen2: Gen1 ELF køyrer bygg_bundle (same moduler) → NCB → host ncb_to_elf.
# Bestått når Gen1.elf og Gen2.elf er identiske (Linux x86-64).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# shellcheck source=omgang6b_compiler_bundle_args.inc.sh
. "$ROOT/tools/omgang6b_compiler_bundle_args.inc.sh"

OS="$(uname -s)"
ARCH="$(uname -m)"

GEN1_NCB="$ROOT/build/6b/compiler_stage0.ncb.json"
GEN1_ELF="$ROOT/build/6b/selfcompile/gen1_compiler.elf"
GEN2_NCB="$ROOT/build/6b/selfcompile/gen2.ncb.json"
GEN2_ELF="$ROOT/build/6b/selfcompile/gen2_compiler.elf"

BUNDLE_ARGS_STR="${OMGANG6B_BUNDLE_ARGS[*]}"

mkdir -p "$ROOT/build/6b/selfcompile"

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

printf '=== Omgang 6b.3: stage-0 ELF sjølvkompilering ===\n\n'

printf '[1/4] Gen1: stage-0 NCB + ELF (host)...\n'
bash "$ROOT/tools/build_omgang6b_compiler_ncb.sh" "$GEN1_NCB"
bash "$ROOT/tools/ncb_to_elf.sh" "$GEN1_NCB" "$GEN1_ELF"
B1="$(wc -c < "$GEN1_ELF" | tr -d ' ')"
printf '  [OK] Gen1 ELF %s bytes\n\n' "$B1"

if [ "$OS" != "Linux" ] || { [ "$ARCH" != "x86_64" ] && [ "$ARCH" != "amd64" ]; }; then
    printf '[2–4/4] Hopp over ELF→ELF (%s/%s — krev Linux x86-64)\n' "$OS" "$ARCH"
    printf '\n=== Omgang 6b.3: DELVIS (host Gen1 bygd) ===\n'
    exit 0
fi

printf '[2/4] Gen2 NCB via Gen1 ELF (bygg_bundle)...\n'
rm -f "$GEN2_NCB"
export NORSCODE_BUNDLE_ARGS="$BUNDLE_ARGS_STR"
export NORSCODE_BUNDLE_OUTPUT="$GEN2_NCB"
if ! "$GEN1_ELF" 2>&1; then
    printf '  [FEIL] Gen1 ELF bundle-modus feila\n' >&2
    exit 1
fi
unset NORSCODE_BUNDLE_ARGS NORSCODE_BUNDLE_OUTPUT
if [ ! -f "$GEN2_NCB" ]; then
    printf '  [FEIL] Gen2 NCB ikkje skrive\n' >&2
    exit 1
fi
bash "$ROOT/tools/patch_ncb_entry.sh" "$GEN2_NCB" "selfhost.elf_compile_driver.start"
N1="$(wc -c < "$GEN1_NCB" | tr -d ' ')"
N2="$(wc -c < "$GEN2_NCB" | tr -d ' ')"
printf '  [OK] Gen2 NCB %s bytes (Gen1 NCB %s bytes)\n' "$N2" "$N1"
if cmp -s "$GEN1_NCB" "$GEN2_NCB"; then
    printf '  [OK] NCB byte-paritet Gen1 == Gen2\n\n'
else
    printf '  [MERK] NCB differ — sjekkar ELF-paritet likevel\n\n'
fi

printf '[3/4] Gen2 ELF frå Gen2 NCB (host codegen)...\n'
bash "$ROOT/tools/ncb_to_elf.sh" "$GEN2_NCB" "$GEN2_ELF"
B2="$(wc -c < "$GEN2_ELF" | tr -d ' ')"
printf '  [OK] Gen2 ELF %s bytes\n\n' "$B2"

printf '[4/4] Byte-paritet Gen1 ELF == Gen2 ELF...\n'
if cmp -s "$GEN1_ELF" "$GEN2_ELF"; then
    printf '  [OK] %s bytes identiske\n\n' "$B1"
    printf '=== Omgang 6b.3: BESTÅTT ===\n'
    exit 0
fi

printf '  [FEIL] ELF differ (gen1=%s, gen2=%s bytes)\n' "$B1" "$B2" >&2
cmp -l "$GEN1_ELF" "$GEN2_ELF" 2>/dev/null | head -5 || true
exit 1
