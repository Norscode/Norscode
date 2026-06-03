#!/usr/bin/env bash
# tools/verify_omgang6.sh — Omgang 6: bygg-native → Linux ELF64 utan clang
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Omgang 6: Native ELF (bygg-native) ===\n\n'

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

SRC="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_XXXXXX.no" 2>/dev/null || printf '%s/nc_omgang6_%s.no' "${TMPDIR:-/tmp}" "$$")"
OUT1="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf1_XXXXXX" 2>/dev/null || printf '%s/nc_omgang6_elf1_%s' "${TMPDIR:-/tmp}" "$$")"
OUT2="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf2_XXXXXX" 2>/dev/null || printf '%s/nc_omgang6_elf2_%s' "${TMPDIR:-/tmp}" "$$")"
trap 'rm -f "$SRC" "$OUT1" "$OUT2"' EXIT

printf 'funksjon start() {\n  skriv("ELF-smoke OK\\n")\n  la x: heltall = 6 * 7\n  skriv(tekst_fra_heltall(x) + "\\n")\n}\n' > "$SRC"

printf '1. bygg-native (Gen1)...\n'
"$ROOT/bin/nc" bygg-native "$SRC" "$OUT1"
BYTES1="$(wc -c < "$OUT1" | tr -d ' ')"
printf '  [OK] %s bytes\n\n' "$BYTES1"

printf '2. bygg-native (Gen2, byte-determinisme)...\n'
"$ROOT/bin/nc" bygg-native "$SRC" "$OUT2"
if ! cmp -s "$OUT1" "$OUT2"; then
    printf '  [FEIL] Gen1 og Gen2 ELF er ikkje identiske\n' >&2
    cmp -l "$OUT1" "$OUT2" 2>/dev/null | head -5 || true
    exit 1
fi
printf '  [OK] Gen1 == Gen2 (%s bytes)\n\n' "$BYTES1"

OS="$(uname -s)"
ARCH="$(uname -m)"
if [ "$OS" = "Linux" ] && { [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "amd64" ]; }; then
    printf '3. Køyr ELF på Linux x86-64...\n'
    chmod +x "$OUT1"
    set +e
    RUN_OUT="$("$OUT1" 2>&1)"
    RUN_RC=$?
    set -e
    if [ -n "$RUN_OUT" ]; then
        printf '%s\n' "$RUN_OUT"
    fi
    if [ "$RUN_RC" -ne 0 ]; then
        printf '  [FEIL] ELF exit code %d\n' "$RUN_RC" >&2
        exit "$RUN_RC"
    fi
    echo "$RUN_OUT" | grep -q "ELF-smoke OK" || { printf '  [FEIL] manglar ELF-smoke OK\n' >&2; exit 1; }
    echo "$RUN_OUT" | grep -q "42" || { printf '  [FEIL] manglar 42\n' >&2; exit 1; }
    printf '  [OK] ELF køyrer korrekt\n\n'
else
    printf '3. Hopp over ELF-køyring (plattform %s/%s — ELF er Linux x86-64)\n\n' "$OS" "$ARCH"
fi

printf '=== Omgang 6: BESTÅTT ===\n'
printf 'bygg-native produserer deterministisk Linux ELF64 utan clang.\n'
