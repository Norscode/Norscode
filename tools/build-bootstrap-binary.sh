#!/usr/bin/env bash
# tools/build-bootstrap-binary.sh
#
# Installerer Norscode native bootstrap-kompilator.
#
# Rekkefølge:
#   1. dist/norcode-bootstrap-compile  finnes allerede → bekreft og ferdig
#   2. Ikke funnet → bygg fra selfhost-kildekode via Python-bootstrap (engangsoperasjon)
#
# Python trengs BARE i trinn 2 (engangsbygging av den native kompilatoren).
# Etter første bygging er Python ikke lenger nødvendig for daglig bruk.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
BUILD_DIR="${ROOT_DIR}/build"
BOOTSTRAP_BIN="${DIST_DIR}/norcode-bootstrap-compile"
LEGACY_LAUNCHER="${DIST_DIR}/norscode"

mkdir -p "${DIST_DIR}" "${BUILD_DIR}"

# ─── Sjekk om native kompilator allerede finnes ─────────────────────────────
if [ -x "${BOOTSTRAP_BIN}" ]; then
    printf 'Native bootstrap-kompilator klar: %s\n' "${BOOTSTRAP_BIN}"
    printf 'Norscode er klar (Python-fri). Kjør: ./bin/nc run app.no\n'
    exit 0
fi

# ─── Bygg native kompilator fra selfhost-kildekode ──────────────────────────
printf 'Native kompilator ikke funnet. Bygger fra selfhost-kildekode...\n'
printf '(Python brukes kun til dette engangssteget.)\n'

if ! command -v python3 >/dev/null 2>&1; then
    printf 'Feil: python3 trengs for å bygge native kompilator første gang.\n' >&2
    printf 'Installer Python 3.10+ og kjør dette scriptet igjen.\n' >&2
    exit 1
fi

python3 "${ROOT_DIR}/main.py" selfhost-bootstrap-gate

BUILT_BIN="${BUILD_DIR}/norcode-bootstrap-compile"
if [ ! -x "${BUILT_BIN}" ]; then
    printf 'Feil: selfhost-bootstrap-gate fullførte, men %s ble ikke generert.\n' "${BUILT_BIN}" >&2
    exit 1
fi

cp "${BUILT_BIN}" "${BOOTSTRAP_BIN}"
chmod +x "${BOOTSTRAP_BIN}"

printf 'Bygde og installerte: %s\n' "${BOOTSTRAP_BIN}"
printf 'Norscode er klar (Python-fri). Kjør: ./bin/nc run app.no\n'

# ─── Fjern gammel C/Python-launcher hvis den eksisterer ─────────────────────
if [ -f "${LEGACY_LAUNCHER}" ]; then
    # Behold for bakoverkompatibilitet, men merk den som utdatert
    printf 'Merk: %s er den gamle Python-launcheren og er ikke lenger nødvendig.\n' "${LEGACY_LAUNCHER}"
fi
