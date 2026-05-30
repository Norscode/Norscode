#!/usr/bin/env bash
# tools/build-bootstrap-binary.sh
#
# Bygger Norscode bootstrap-infrastruktur:
#   1. Bygg dist/nc-vm frå tools/nc_vm.c (C, eingongsbruk)
#   2. Køyr selfhost bootstrap gate (verifisering)
#   3. Bygg dist/norscode_native (native Norscode-binary via NCB→C→clang)
#
# Norscode er klar når dist/norscode_native finst.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
NC_VM="${DIST_DIR}/nc-vm"
NC_NATIVE="${DIST_DIR}/norscode_native"

mkdir -p "${DIST_DIR}"

# ─── Bygg nc-vm frå kilde (trengst for bootstrapping) ───────────────────────
if [ ! -x "${NC_VM}" ]; then
    printf 'Byggjer dist/nc-vm frå tools/nc_vm.c...\n'
    CC="${CC:-clang}"
    if ! command -v "$CC" >/dev/null 2>&1; then CC=cc; fi
    if ! command -v "$CC" >/dev/null 2>&1; then
        printf 'Feil: trenger clang eller cc for å byggje nc-vm.\n' >&2
        exit 1
    fi
    "$CC" -O2 -o "${NC_VM}" "${ROOT_DIR}/tools/nc_vm.c"
    printf '✓ dist/nc-vm bygget med %s\n' "$CC"
else
    printf '✓ dist/nc-vm allereie klar\n'
fi

# ─── Selfhost bootstrap gate — verifisering ─────────────────────────────────
printf 'Køyrer selfhost bootstrap gate...\n'
if ! "${ROOT_DIR}/bin/nc" selfhost-bootstrap-gate; then
    printf 'Feil: selfhost bootstrap gate feila.\n' >&2
    exit 1
fi
printf '✓ Bootstrap gate: BESTÅTT\n'

# ─── Bygg norscode_native via tools/build_norscode_native.sh ─────────────────
if [ ! -x "${NC_NATIVE}" ] || [ "${ROOT_DIR}/bootstrap/kompiler.ncb.json" -nt "${NC_NATIVE}" ]; then
    printf 'Byggjer dist/norscode_native...\n'
    if bash "${ROOT_DIR}/tools/build_norscode_native.sh"; then
        printf '✓ dist/norscode_native klar\n'
    else
        printf 'Advarsel: norscode_native bygg feila, brukar nc-vm som fallback\n' >&2
    fi
else
    printf '✓ dist/norscode_native allereie klar\n'
fi

# ─── Lag shell-wrapper for bakoverkompatibilitet ─────────────────────────────
BOOTSTRAP_BIN="${DIST_DIR}/norcode-bootstrap-compile"
cp "${ROOT_DIR}/tools/bootstrap_wrapper.sh" "${BOOTSTRAP_BIN}"
chmod +x "${BOOTSTRAP_BIN}"
printf '✓ dist/norcode-bootstrap-compile (shell-wrapper) oppdatert\n'

printf 'Norscode er klar. Køyr: ./bin/nc run app.no\n'
