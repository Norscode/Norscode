#!/usr/bin/env bash
# tools/build-bootstrap-binary.sh
#
# Bygger/oppdaterer Norscode bootstrap-kompilator.
#
# Pipeline:
#   1. Sjekk om dist/norcode-bootstrap-compile allereie er OK
#   2. Bygg dist/nc-vm frå tools/nc_vm.c (clang/cc) om ikkje tilgjengeleg
#   3. Køyr selfhost bootstrap gate Python-fri via nc-vm
#   4. Generer Python VM-wrapper som dist/norcode-bootstrap-compile
#      (fallback for selfcheck/identity via NATIVE_BIN-protokollen)
#
# Python trengst IKKJE for bootstrap-gate — berre for wrapper-generering.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
BOOTSTRAP_BIN="${DIST_DIR}/norcode-bootstrap-compile"
NC_VM="${DIST_DIR}/nc-vm"
WRAPPER_SRC="${ROOT_DIR}/tools/bootstrap_wrapper.py"

mkdir -p "${DIST_DIR}"

# ─── Sjekk om binaryen allereie er OK ───────────────────────────────────────
if [ -x "${BOOTSTRAP_BIN}" ]; then
    printf 'Bootstrap-kompilator klar: %s\n' "${BOOTSTRAP_BIN}"
    printf 'Norscode er klar. Køyr: ./bin/nc run app.no\n'
    exit 0
fi

# ─── Bygg nc-vm frå kilde om ikkje tilgjengeleg ─────────────────────────────
if [ ! -x "${NC_VM}" ]; then
    printf 'Byggjer dist/nc-vm frå tools/nc_vm.c...\n'
    CC="${CC:-clang}"
    if ! command -v "$CC" >/dev/null 2>&1; then CC=cc; fi
    if ! command -v "$CC" >/dev/null 2>&1; then
        printf 'Feil: trenger clang eller cc for å byggje nc-vm.\n' >&2
        printf 'Installer ein C-kompilator og køyr scriptet igjen.\n' >&2
        exit 1
    fi
    "$CC" -O2 -o "${NC_VM}" "${ROOT_DIR}/tools/nc_vm.c"
    printf '✓ dist/nc-vm bygget med %s\n' "$CC"
fi

# ─── Selfhost bootstrap gate — Python-fri via nc-vm ─────────────────────────
printf 'Køyrer selfhost bootstrap gate (Python-fri)...\n'
if ! "${ROOT_DIR}/bin/nc" selfhost-bootstrap-gate; then
    printf 'Feil: selfhost bootstrap gate feila.\n' >&2
    exit 1
fi
printf 'Bootstrap gate: BESTÅTT\n'

# ─── Generer Python VM-wrapper (for NATIVE_BIN selfcheck/identity) ──────────
printf 'Genererer Python VM-wrapper: %s\n' "${BOOTSTRAP_BIN}"

if [ ! -f "${WRAPPER_SRC}" ]; then
    printf 'Feil: wrapper-mal ikkje funnen: %s\n' "${WRAPPER_SRC}" >&2
    exit 1
fi

cp "${WRAPPER_SRC}" "${BOOTSTRAP_BIN}"
chmod +x "${BOOTSTRAP_BIN}"

if [ ! -x "${BOOTSTRAP_BIN}" ]; then
    printf 'Feil: klarte ikkje å installere wrapper: %s\n' "${BOOTSTRAP_BIN}" >&2
    exit 1
fi

# ─── Røyk-test av wrapper ────────────────────────────────────────────────────
printf 'Røyk-test av Python VM-wrapper...\n'
NORCODE_BOOTSTRAP_VM=1 NORCODE_BOOTSTRAP_CLI=1 \
NORCODE_ARGC=1 NORCODE_ARG0=selfcheck \
"${BOOTSTRAP_BIN}"

printf 'Bygde og installerte: %s\n' "${BOOTSTRAP_BIN}"
printf 'Norscode er klar. Køyr: ./bin/nc run app.no\n'
