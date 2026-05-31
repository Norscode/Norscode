#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Bygger dist/norscode_native frå pre-genererte C-filer i bootstrap/c/.
# Krev KUN ein C-kompilator (clang eller cc) — IKKJE nc-vm eller Python.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norscode_native"
CC="${CC:-clang}"
if ! command -v "$CC" >/dev/null 2>&1; then CC=cc; fi
if ! command -v "$CC" >/dev/null 2>&1; then
    printf "Feil: trenger clang eller cc\n" >&2; exit 1
fi

mkdir -p "${ROOT}/dist"

# ─── Regenerer C-filer frå aktuell bootstrap-NCB om nødvendig ───────────────
_should_regen=0
if [ ! -f "${ROOT}/bootstrap/c/norscode_generated.c" ]; then
    _should_regen=1
elif [ "${ROOT}/bootstrap/kompiler.ncb.json" -nt "${ROOT}/bootstrap/c/norscode_generated.c" ]; then
    _should_regen=1
fi

if [ "$_should_regen" = "1" ] && [ -x "${ROOT}/dist/norscode_native" ]; then
    printf "Regenererer C frå bootstrap/kompiler.ncb.json...\n"
    env NORSCODE_CMD=run NORSCODE_FILE="${ROOT}/selfhost/ncb_to_c.no" \
        NC_NCB_INPUT="${ROOT}/bootstrap/kompiler.ncb.json" \
        NC_C_OUTPUT="${ROOT}/bootstrap/c/norscode_generated.c" \
        "${ROOT}/dist/norscode_native" 2>/dev/null || true
    python3 "${ROOT}/tools/gen_dispatch.py" "${ROOT}/bootstrap/kompiler.ncb.json" \
        > "${ROOT}/bootstrap/c/nc_dispatch.c" 2>/dev/null || true
fi

printf "Kompilerer norscode_native...\n"
TMP="$(mktemp /tmp/nc_native_$$.c 2>/dev/null || echo /tmp/nc_native_$$.c)"
cat "${ROOT}/tools/nc_runtime_mini.c" > "$TMP"
cat "${ROOT}/bootstrap/c/nc_dispatch.c" >> "$TMP"
grep -v '#include.*nc_runtime' "${ROOT}/bootstrap/c/norscode_generated.c" | \
    sed 's/^int main/static int nc_gen_main/' >> "$TMP"
cat "${ROOT}/tools/nc_native_main.c" >> "$TMP"
"$CC" -O2 -Wno-everything -o "$OUT" "$TMP"
rm -f "$TMP"

printf "✓ dist/norscode_native bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
