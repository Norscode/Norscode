#!/usr/bin/env bash
# tools/build-bootstrap-binary.sh
#
# Bygger Norscode bootstrap-infrastruktur.
#
# Rekkefølge:
#   1. Bygg dist/nc-vm frå C (berre om norscode_native IKKJE finst)
#   2. Bygg dist/norscode_native frå bootstrap/c/ (clang, ingen Python)
#   3. Køyr selfhost bootstrap gate (verifisering)
#   4. Lag dist/norscode-bootstrap-compile (shell-wrapper)
#
# ETTER FØRSTE BYGG: berre steg 2-4 er nødvendig.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
NC_VM="${DIST_DIR}/nc-vm"
NC_NATIVE="${DIST_DIR}/norscode_native"

mkdir -p "${DIST_DIR}"

# ─── Steg 1: Bygg nc-vm KUN om norscode_native IKKJE finst ──────────────────
if [ ! -x "${NC_NATIVE}" ]; then
    if [ ! -x "${NC_VM}" ]; then
        printf 'Byggjer dist/nc-vm (eingongsbruk for bootstrapping)...\n'
        CC="${CC:-clang}"
        if ! command -v "$CC" >/dev/null 2>&1; then CC=cc; fi
        if ! command -v "$CC" >/dev/null 2>&1; then
            printf 'Feil: trenger clang eller cc\n' >&2; exit 1
        fi
        "$CC" -O2 -o "${NC_VM}" "${ROOT_DIR}/tools/nc_vm.c"
        printf '✓ dist/nc-vm bygget (bootstrap-eingongsbruk)\n'
    fi
fi

# ─── Steg 2: Bygg norscode_native ────────────────────────────────────────────
if [ ! -x "${NC_NATIVE}" ] || \
   [ "${ROOT_DIR}/bootstrap/kompiler.ncb.json" -nt "${NC_NATIVE}" ] || \
   [ "${ROOT_DIR}/bootstrap/c/norscode_generated.c" -nt "${NC_NATIVE}" ]; then
    printf 'Byggjer dist/norscode_native...\n'
    bash "${ROOT_DIR}/tools/build_norscode_native.sh"
else
    printf '✓ dist/norscode_native allereie klar\n'
fi

# ─── Steg 3: Selfhost bootstrap gate ─────────────────────────────────────────
printf 'Køyrer selfhost bootstrap gate...\n'
if ! "${ROOT_DIR}/bin/nc" selfhost-bootstrap-gate; then
    printf 'Feil: selfhost bootstrap gate feila.\n' >&2; exit 1
fi
printf '✓ Bootstrap gate: BESTÅTT\n'

# ─── Steg 4: Shell-wrapper for bakoverkompatibilitet ─────────────────────────
BOOTSTRAP_BIN="${DIST_DIR}/norscode-bootstrap-compile"
cp "${ROOT_DIR}/tools/bootstrap_wrapper.sh" "${BOOTSTRAP_BIN}"
chmod +x "${BOOTSTRAP_BIN}"
printf '✓ dist/norscode-bootstrap-compile (shell-wrapper) oppdatert\n'

printf 'Norscode er klar. Køyr: ./bin/nc run app.no\n'
