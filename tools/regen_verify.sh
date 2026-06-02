#!/usr/bin/env bash
# tools/regen_verify.sh — L6: deterministisk regen (to kjøringar, same SHA-256)
#
# Krever seed (dist/norscode_native). Endrar ikkje committed bootstrap/c (generert lokalt).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

sha_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        shasum -a 256 "$1" | awk '{print $1}'
    fi
}

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

REGEN_A="$ROOT/build/regen_verify_a"
REGEN_B="$ROOT/build/regen_verify_b"
rm -rf "$REGEN_A" "$REGEN_B"
mkdir -p "$REGEN_A" "$REGEN_B"

printf '=== Regen-verify (L6: determinisme) ===\n\n'

printf '[1/2] Fyrste regen...\n'
REGEN_ROOT="$REGEN_A" bash "$ROOT/tools/regen_native.sh"
printf '\n[2/2] Andre regen...\n'
REGEN_ROOT="$REGEN_B" bash "$ROOT/tools/regen_native.sh"

printf '\nSamanliknar SHA-256 mellom to regen-køyringar...\n'
OK=1

compare() {
    local label="$1"
    local a="$2"
    local b="$3"
    local ha hb
    ha="$(sha_file "$a")"
    hb="$(sha_file "$b")"
    if [ "$ha" = "$hb" ]; then
        printf '  [OK] %s\n' "$label"
    else
        printf '  [AVVIK] %s\n' "$label"
        printf '    run A: %s\n' "$ha"
        printf '    run B: %s\n' "$hb"
        OK=0
    fi
}

compare "kompiler.ncb.json" \
    "$REGEN_A/kompiler.ncb.json" "$REGEN_B/kompiler.ncb.json"
compare "norscode_generated.c" \
    "$REGEN_A/c/norscode_generated.c" "$REGEN_B/c/norscode_generated.c"
compare "nc_dispatch.c" \
    "$REGEN_A/c/nc_dispatch.c" "$REGEN_B/c/nc_dispatch.c"

printf '\n'
if [ "$OK" -eq 1 ]; then
    printf '=== Regen-verify: BESTÅTT (deterministisk regen) ===\n'
    exit 0
fi

printf '=== Regen-verify: AVVIK ===\n'
exit 1
