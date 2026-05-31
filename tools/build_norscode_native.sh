#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Bygger dist/norscode_native frå pre-genererte C-filer i bootstrap/c/.
# Krev KUN ein C-kompilator (clang eller cc).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norscode_native"
CC="${CC:-clang}"
if ! command -v "$CC" >/dev/null 2>&1; then CC=cc; fi
if ! command -v "$CC" >/dev/null 2>&1; then
    printf "Feil: trenger clang eller cc\n" >&2; exit 1
fi
mkdir -p "${ROOT}/dist"

# ─── Regenerer C-filer om nødvendig ─────────────────────────────────────────
if [ -x "${ROOT}/dist/norscode_native" ] && \
   [ "${ROOT}/bootstrap/kompiler.ncb.json" -nt "${ROOT}/bootstrap/c/norscode_generated.c" ]; then
    printf "Regenererer C frå oppdatert NCB...\n"
    env NORSCODE_CMD=run NORSCODE_FILE="${ROOT}/selfhost/ncb_to_c.no" \
        NC_NCB_INPUT="${ROOT}/bootstrap/kompiler.ncb.json" \
        NC_C_OUTPUT="${ROOT}/bootstrap/c/norscode_generated.c" \
        "${ROOT}/dist/norscode_native" 2>/dev/null || true
    env NORSCODE_CMD=run NORSCODE_FILE="${ROOT}/selfhost/gen_dispatch.no" \
        NC_NCB_INPUT="${ROOT}/bootstrap/kompiler.ncb.json" \
        NC_DISPATCH_OUTPUT="${ROOT}/bootstrap/c/nc_dispatch.c" \
        "${ROOT}/dist/norscode_native" 2>/dev/null || true
fi

printf "Kompilerer norscode_native...\n"
TMP="$(mktemp /tmp/nc_native_$$.c 2>/dev/null || echo /tmp/nc_native_$$.c)"

# Sjekk om runtime er embedded i generated.c
if grep -q "nc_runtime_mini.c embedded" "${ROOT}/bootstrap/c/norscode_generated.c" 2>/dev/null; then
    # Runtime embedded — sett dispatch ETTER, native_main sist
    grep -v '#include.*nc_runtime' "${ROOT}/bootstrap/c/norscode_generated.c" |
        sed 's/^int main/static int nc_gen_main/' > "$TMP"
    cat "${ROOT}/bootstrap/c/nc_dispatch.c" >> "$TMP"
    cat "${ROOT}/tools/nc_native_main.c" >> "$TMP"
else
    # Runtime IKKJE embedded — legg til separat
    cat "${ROOT}/tools/nc_runtime_mini.c" > "$TMP"
    cat "${ROOT}/bootstrap/c/nc_dispatch.c" >> "$TMP"
    grep -v '#include.*nc_runtime' "${ROOT}/bootstrap/c/norscode_generated.c" |
        sed 's/^int main/static int nc_gen_main/' >> "$TMP"
    cat "${ROOT}/tools/nc_native_main.c" >> "$TMP"
fi

"$CC" -O2 -Wno-everything -o "$OUT" "$TMP"
rm -f "$TMP"
printf "✓ dist/norscode_native bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
