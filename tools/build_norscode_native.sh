#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Bygger dist/norscode_native frå pre-genererte C-filer i bootstrap/c/.
# Krev KUN ein C-kompilator (clang eller cc) — IKKJE nc-vm eller Python.
#
# For å regenerere bootstrap/c/-filane (treng nc-vm eller eksisterande norscode_native):
#   NC_NCB_INPUT=bootstrap/kompiler.ncb.json ./bin/nc run selfhost/ncb_to_c.no
#   python3 tools/gen_dispatch.py > bootstrap/c/nc_dispatch.c
#
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

if [ "$_should_regen" = "1" ]; then
    printf "Regenererer C frå bootstrap/kompiler.ncb.json...\n"
    _runner=""
    if [ -x "${ROOT}/dist/norscode_native" ]; then
        _runner="${ROOT}/dist/norscode_native"
    else
        printf "Advarsel: bruker eksisterande C-filer (ingen runner funnen)\n" >&2
    fi

    if [ -n "$_runner" ]; then
        env NORSCODE_CMD=run NORSCODE_FILE="${ROOT}/selfhost/ncb_to_c.no" \
            NC_NCB_INPUT="${ROOT}/bootstrap/kompiler.ncb.json" \
            NC_C_OUTPUT="${ROOT}/bootstrap/c/norscode_generated.c" \
            "$_runner" 2>/dev/null || true

        # Generer dispatch-tabell
        python3 "${ROOT}/tools/gen_dispatch.py" \
            "${ROOT}/bootstrap/kompiler.ncb.json" \
            > "${ROOT}/bootstrap/c/nc_dispatch.c" 2>/dev/null || true
    fi
fi

printf "Kompilerer norscode_native...\n"
TMP="$(mktemp /tmp/nc_native_$$.c 2>/dev/null || echo /tmp/nc_native_$$.c)"
cat "${ROOT}/tools/nc_runtime_mini.c" > "$TMP"
cat "${ROOT}/bootstrap/c/nc_dispatch.c" >> "$TMP"
# norscode_generated.c inneheld no nc_main.start som main()-inngangspunkt
# nc_native_main.c er ikkje lenger nødvendig
grep -v '#include.*nc_runtime' "${ROOT}/bootstrap/c/norscode_generated.c" >> "$TMP"
"$CC" -O2 -Wno-everything -o "$OUT" "$TMP"
rm -f "$TMP"

printf "✓ dist/norscode_native bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
