#!/usr/bin/env bash
# tools/verify_omgang6.sh — Omgang 6: bygg-native → Linux ELF64 utan clang
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
MODE="${1:-all}"

printf '=== Omgang 6: Native ELF (bygg-native) ===\n\n'

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

SRC1="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_XXXXXX.no" 2>/dev/null || printf '%s/nc_omgang6_%s.no' "${TMPDIR:-/tmp}" "$$")"
SRC2="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_XXXXXX.no" 2>/dev/null || printf '%s/nc_omgang6_%s.no' "${TMPDIR:-/tmp}" "$$")"
OUT1="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf1_XXXXXX" 2>/dev/null || printf '%s/nc_omgang6_elf1_%s' "${TMPDIR:-/tmp}" "$$")"
OUT2="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf2_XXXXXX" 2>/dev/null || printf '%s/nc_omgang6_elf2_%s' "${TMPDIR:-/tmp}" "$$")"
OUT3="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf3_XXXXXX" 2>/dev/null || printf '%s/nc_omgang6_elf3_%s' "${TMPDIR:-/tmp}" "$$")"
trap 'rm -f "$SRC1" "$SRC2" "$OUT1" "$OUT2" "$OUT3"' EXIT

printf 'funksjon start() {\n  skriv("ELF-smoke OK\\n")\n}\n' > "$SRC1"
printf 'funksjon start() {\n  la x: heltall = 6 * 7\n  skriv(tekst_fra_heltall(x) + "\\n")\n}\n' > "$SRC2"

if [ "$MODE" = "skriv" ] || [ "$MODE" = "all" ]; then
printf '1. bygg-native (Gen1, skriv-smoke)...\n'
"$ROOT/bin/nc" bygg-native "$SRC1" "$OUT1"
BYTES1="$(wc -c < "$OUT1" | tr -d ' ')"
printf '  [OK] %s bytes\n\n' "$BYTES1"

printf '2. bygg-native (Gen2, byte-determinisme, skriv-smoke)...\n'
"$ROOT/bin/nc" bygg-native "$SRC1" "$OUT2"
if ! cmp -s "$OUT1" "$OUT2"; then
    printf '  [FEIL] Gen1 og Gen2 ELF er ikkje identiske\n' >&2
    cmp -l "$OUT1" "$OUT2" 2>/dev/null | head -5 || true
    exit 1
fi
printf '  [OK] Gen1 == Gen2 (%s bytes)\n\n' "$BYTES1"
fi

OS="$(uname -s)"
ARCH="$(uname -m)"
if [ "$OS" = "Linux" ] && { [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "amd64" ]; }; then
    if [ "$MODE" = "skriv" ] || [ "$MODE" = "all" ]; then
    printf '3. Køyr skriv-smoke på Linux x86-64...\n'
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
    printf '  [OK] skriv-smoke køyrer korrekt\n\n'
    fi

    if [ "$MODE" = "int" ] || [ "$MODE" = "all" ]; then
    printf '4. bygg-native (Gen1, int-to-str smoke)...\n'
    "$ROOT/bin/nc" bygg-native "$SRC2" "$OUT3"
    BYTES3="$(wc -c < "$OUT3" | tr -d ' ')"
    printf '  [OK] %s bytes\n\n' "$BYTES3"

    printf '5. Køyr int-to-str smoke på Linux x86-64...\n'
    chmod +x "$OUT3"
    set +e
    RUN_OUT="$("$OUT3" 2>&1)"
    RUN_RC=$?
    set -e
    if [ -n "$RUN_OUT" ]; then
        printf '%s\n' "$RUN_OUT"
    fi
    if [ "$RUN_RC" -ne 0 ]; then
        printf '  [FEIL] ELF exit code %d\n' "$RUN_RC" >&2
        exit "$RUN_RC"
    fi
    echo "$RUN_OUT" | grep -q "42" || { printf '  [FEIL] manglar 42\n' >&2; exit 1; }
    printf '  [OK] int-to-str smoke køyrer korrekt\n\n'
    fi
else
    printf '3. Hopp over ELF-køyring (plattform %s/%s — ELF er Linux x86-64)\n\n' "$OS" "$ARCH"
fi

printf '=== Omgang 6: BESTÅTT ===\n'
printf 'bygg-native produserer deterministisk Linux ELF64 utan clang.\n'
