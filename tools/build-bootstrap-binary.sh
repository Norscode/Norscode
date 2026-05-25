#!/usr/bin/env bash
# tools/build-bootstrap-binary.sh
#
# Bygger/oppdaterer Norscode native bootstrap-kompilator.
#
# Ny pipeline (ingen C-kompilator):
#   1. Sjekk om dist/norcode-bootstrap-compile allerede finnes og er OK
#   2. Køyr selfhost bootstrap gate  (./bin/nc --python-fallback selfhost-bootstrap-gate)
#   3. Generer Python VM-wrapper som dist/norcode-bootstrap-compile
#
# Python trengst berre til gate og wrapper-generering.
# Ingen clang / gcc / cc nødvendig.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
BOOTSTRAP_BIN="${DIST_DIR}/norcode-bootstrap-compile"
WRAPPER_SRC="${ROOT_DIR}/tools/bootstrap_wrapper.py"

mkdir -p "${DIST_DIR}"

# ─── Sjekk om Python er tilgjengeleg ────────────────────────────────────────
if ! command -v python3 >/dev/null 2>&1; then
    printf 'Feil: python3 trengs for å generere bootstrap-kompilator.\n' >&2
    printf 'Installer Python 3.10+ og kjør dette scriptet igjen.\n' >&2
    exit 1
fi

# ─── Sjekk om binaryen allerede er OK ───────────────────────────────────────
_bootstrap_ok() {
    [ -x "${BOOTSTRAP_BIN}" ] && return 0 || return 1
}

if _bootstrap_ok; then
    printf 'Bootstrap-kompilator klar: %s\n' "${BOOTSTRAP_BIN}"
    printf 'Norscode er klar. Kjør: ./bin/nc run app.no\n'
    exit 0
fi

# ─── Selfhost bootstrap gate ─────────────────────────────────────────────────
printf 'Køyrer selfhost bootstrap gate...\n'
if ! bash "${ROOT_DIR}/bin/nc" --python-fallback selfhost-bootstrap-gate; then
    printf 'Feil: selfhost bootstrap gate feila.\n' >&2
    exit 1
fi
printf 'Bootstrap gate: BESTÅTT\n'

# ─── Generer Python VM-wrapper ───────────────────────────────────────────────
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
printf 'Norscode er klar (ingen C-kompilator). Kjør: ./bin/nc run app.no\n'
