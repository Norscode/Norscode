#!/usr/bin/env bash
# tools/build-bootstrap-binary.sh
#
# Installerer Norscode native bootstrap-kompilator.
#
# Rekkefølge:
#   1. dist/norcode-bootstrap-compile  finnes allerede → bekreft og ferdig
#   2. Ikke funnet → kjør selfhost-gate for å sikre de genererte NCBB-dataene
#   3. Bygg host-native bootstrap-kompilator fra C VM-kildene
#
# Python trengs kun for selfhost-gaten som produserer de genererte input-blokkene.
# Selve bootstrap-kompilatoren bygges deretter med host-kompilatoren.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
BUILD_DIR="${ROOT_DIR}/build"
BOOTSTRAP_BIN="${DIST_DIR}/norcode-bootstrap-compile"
LEGACY_LAUNCHER="${DIST_DIR}/norscode"
BOOTSTRAP_DATA="${BUILD_DIR}/bootstrap_compiler_bundle_ncb_data.c"
NATIVE_DATA="${BUILD_DIR}/native_elf_compiler_bundle_ncb_data.c"
BOOTSTRAP_MANIFEST="${ROOT_DIR}/tools/c_minimal_vm/bootstrap_stub_manifest.h"
BOOTSTRAP_DISPATCH_INC="${BUILD_DIR}/mv_bootstrap_stub_manifest_dispatch.inc"

if command -v clang >/dev/null 2>&1; then
    HOST_CC="clang"
elif command -v cc >/dev/null 2>&1; then
    HOST_CC="cc"
elif command -v gcc >/dev/null 2>&1; then
    HOST_CC="gcc"
else
    printf 'Feil: ingen host-kompilator funnet (trengte clang/cc/gcc).\n' >&2
    exit 1
fi

mkdir -p "${DIST_DIR}" "${BUILD_DIR}"

# ─── Sjekk om native kompilator allerede finnes ─────────────────────────────
if [ -x "${BOOTSTRAP_BIN}" ]; then
    printf 'Native bootstrap-kompilator klar: %s\n' "${BOOTSTRAP_BIN}"
    printf 'Norscode er klar (Python-fri). Kjør: ./bin/nc run app.no\n'
    exit 0
fi

# ─── Bygg native kompilator fra selfhost-kildekode ──────────────────────────
printf 'Native kompilator ikke funnet. Bygger fra selfhost-kildekode...\n'
printf '(Python brukes kun til å forberede selfhost-inputtene.)\n'

if ! command -v python3 >/dev/null 2>&1; then
    printf 'Feil: python3 trengs for å bygge native kompilator første gang.\n' >&2
    printf 'Installer Python 3.10+ og kjør dette scriptet igjen.\n' >&2
    exit 1
fi

python3 "${ROOT_DIR}/main.py" selfhost-bootstrap-gate

if [ ! -f "${BOOTSTRAP_DATA}" ] || [ ! -f "${NATIVE_DATA}" ]; then
    printf 'Feil: selfhost-bootstrap-gate fullførte, men genererte ikke C-dataene som trengs for bootstrap-build.\n' >&2
    printf 'Mangler: %s og/eller %s\n' "${BOOTSTRAP_DATA}" "${NATIVE_DATA}" >&2
    exit 1
fi

awk -F'"' '
    /^    "/ {
        print "    if (streq(name, \"" $2 "\")) return stub_noop_true(rt, out);"
    }
' "${BOOTSTRAP_MANIFEST}" > "${BOOTSTRAP_DISPATCH_INC}"

printf 'Bygger host-native bootstrap-kompilator med %s...\n' "${HOST_CC}"

"${HOST_CC}" -Wall -Wextra -O2 \
    -DNORCODE_DUAL_NCBB \
    -DNORCODE_BOOTSTRAP_STUBS \
    -DNORCODE_EMBED_NATIVE_NCBB \
    -DNORCODE_BOOTSTRAP_HOST_EXEC \
    -I "${ROOT_DIR}/tools/c_minimal_vm" \
    -I "${BUILD_DIR}" \
    -o "${BOOTSTRAP_BIN}" \
    "${ROOT_DIR}/tools/c_minimal_vm/bootstrap_compiler_probe_main.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/compiler_standalone.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/norcode_p3b_compiler_markers.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/mv_arena.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/minimal_vm.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/mv_builtins.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/mv_syscall.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/mv_io.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/mv_env.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/ncbb_loader.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/mv_bootstrap_stubs.c" \
    "${ROOT_DIR}/tools/c_minimal_vm/mv_bootstrap_host_exec.c" \
    "${NATIVE_DATA}" \
    "${BOOTSTRAP_DATA}"

chmod +x "${BOOTSTRAP_BIN}"

if [ ! -x "${BOOTSTRAP_BIN}" ]; then
    printf 'Feil: bygde bootstrap-binaryen, men den ble ikke kjørbar: %s\n' "${BOOTSTRAP_BIN}" >&2
    exit 1
fi

NORCODE_BOOTSTRAP_VM=1 NORCODE_BOOTSTRAP_CLI=1 \
NORCODE_ARGC=1 NORCODE_ARG0=selfcheck \
"${BOOTSTRAP_BIN}"

printf 'Bygde og installerte: %s\n' "${BOOTSTRAP_BIN}"
printf 'Norscode er klar (Python-fri). Kjør: ./bin/nc run app.no\n'

# ─── Fjern gammel C/Python-launcher hvis den eksisterer ─────────────────────
if [ -f "${LEGACY_LAUNCHER}" ]; then
    # Behold for bakoverkompatibilitet, men merk den som utdatert
    printf 'Merk: %s er den gamle Python-launcheren og er ikke lenger nødvendig.\n' "${LEGACY_LAUNCHER}"
fi
