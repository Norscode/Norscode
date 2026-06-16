#!/bin/sh
# tools/maint/bootstrap.sh — historisk bootstrap via stage-0 / bootstrap-c
#
# Historisk vedlikehaldsskript for overgangs-/seed-spor.
# Bygger dist/norscode_native frå pre-genererte vedlikehaldsfiler i vald BOOTSTRAP_C_ROOT
# (standard: repoets bootstrap/ som historisk kompatibilitetsflate).
# Dette er ikkje normal build/CI-flyt; normalvegen er seed frå bootstrap/stage0/ eller release.
# Krev KUN: clang (eller cc)
#
# Bruk:
#   sh tools/maint/bootstrap.sh
#   ./bin/nc run program.no

set -e

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$ROOT"

CC="${CC:-clang}"
if ! command -v "$CC" >/dev/null 2>&1; then CC=cc; fi
if ! command -v "$CC" >/dev/null 2>&1; then
    printf 'bootstrap: trenger clang eller cc\n' >&2; exit 1
fi

# ── Bygg norscode_native ─────────────────────────────────────────────────────
bash "$ROOT/tools/build_norscode_native.sh"

# ── Røyktest ─────────────────────────────────────────────────────────────────
printf 'Røyktester...\n'
printf 'funksjon start() { skriv("bootstrap OK") }' > /tmp/_nc_bootstrap_smoke.no
RESULT=$(NORSCODE_CMD=run NORSCODE_FILE=/tmp/_nc_bootstrap_smoke.no \
    "$ROOT/dist/norscode_native" 2>/dev/null || echo "")
if [ "$RESULT" = "bootstrap OK" ]; then
    printf '✓ Røyktest bestått\n'
else
    printf '✗ Røyktest feilet (fikk: "%s")\n' "$RESULT" >&2; exit 1
fi

printf '\n✓ Norscode bootstrap fullført.\n'
printf 'Bruk: ./bin/nc run program.no\n'
