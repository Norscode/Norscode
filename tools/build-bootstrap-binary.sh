#!/usr/bin/env bash
# tools/build-bootstrap-binary.sh
#
# Bygger Norscode bootstrap-infrastruktur.
# Treng KUN: clang (eller cc) — ingen nc-vm, ingen Python.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
NC_NATIVE="${DIST_DIR}/norscode_native"

mkdir -p "${DIST_DIR}"

# ─── Bygg norscode_native frå bootstrap/c/ ───────────────────────────────────
if [ ! -x "${NC_NATIVE}" ] || \
   [ "${ROOT_DIR}/bootstrap/kompiler.ncb.json" -nt "${NC_NATIVE}" ] || \
   [ "${ROOT_DIR}/bootstrap/c/norscode_generated.c" -nt "${NC_NATIVE}" ]; then
    printf 'Byggjer dist/norscode_native...\n'
    bash "${ROOT_DIR}/tools/build_norscode_native.sh"
else
    printf '✓ dist/norscode_native allereie klar\n'
fi

# ─── Selfhost bootstrap gate ─────────────────────────────────────────────────
printf 'Køyrer selfhost bootstrap gate...\n'
if ! "${ROOT_DIR}/bin/nc" selfhost-bootstrap-gate; then
    printf 'Feil: selfhost bootstrap gate feila.\n' >&2; exit 1
fi
printf '✓ Bootstrap gate: BESTÅTT\n'

# ─── Shell-wrapper for bakoverkompatibilitet ─────────────────────────────────
BOOTSTRAP_BIN="${DIST_DIR}/norscode-bootstrap-compile"
cp "${ROOT_DIR}/tools/bootstrap_wrapper.sh" "${BOOTSTRAP_BIN}"
chmod +x "${BOOTSTRAP_BIN}"
printf '✓ dist/norscode-bootstrap-compile (shell-wrapper) oppdatert\n'

printf 'Norscode er klar. Køyr: ./bin/nc run app.no\n'
