#!/bin/sh
# bootstrap.sh — bygg Norscode uten Python
#
# Krever kun: clang (eller cc) og bootstrap/kompiler.ncb.json
# Etter kjøring: dist/nc-vm er klar til bruk
#
# Bruk:
#   sh tools/bootstrap.sh
#   ./dist/nc-vm --nc-run program.no

set -e

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

# ── Finn kompilator ──────────────────────────────────────────────────────────
CC="${CC:-clang}"
if ! command -v "$CC" >/dev/null 2>&1; then
    CC=cc
fi
if ! command -v "$CC" >/dev/null 2>&1; then
    printf 'bootstrap: trenger clang eller cc\n' >&2
    exit 1
fi

# ── Bygg nc-vm fra kilde ─────────────────────────────────────────────────────
printf 'Bygger dist/nc-vm med %s...\n' "$CC"
mkdir -p dist
"$CC" -O2 -o dist/nc-vm tools/nc_vm.c
printf '✓ dist/nc-vm bygget\n'

# ── Verifiser bootstrap/kompiler.ncb.json ───────────────────────────────────
if [ ! -f bootstrap/kompiler.ncb.json ]; then
    printf '✗ bootstrap/kompiler.ncb.json mangler!\n' >&2
    printf '  Kjør: python3 tools/nc_precompile.py  (én gang)\n' >&2
    exit 1
fi
printf '✓ bootstrap/kompiler.ncb.json funnet\n'

# ── Røyktest ─────────────────────────────────────────────────────────────────
printf 'Røyktester...\n'
RESULT=$(printf 'funksjon start() -> heltall { skriv("bootstrap OK") returner 0 }' \
    | (cat > /tmp/_nc_bootstrap_smoke.no && ./dist/nc-vm --nc-run /tmp/_nc_bootstrap_smoke.no 2>/dev/null))
if [ "$RESULT" = "bootstrap OK" ]; then
    printf '✓ Røyktest bestått\n'
else
    printf '✗ Røyktest feilet (fikk: %s)\n' "$RESULT" >&2
    exit 1
fi

printf '\nNorscode bootstrap fullført.\n'
printf 'Bruk: ./dist/nc-vm --nc-run program.no\n'
printf 'Eller: ./bin/nc run program.no\n'
