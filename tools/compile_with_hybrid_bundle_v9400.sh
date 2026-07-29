#!/usr/bin/env sh
set -eu

# Norscode-first wrapper: eigarlogikken ligg i compile_with_hybrid_bundle_v9400.no.
# Kompatibilitetsinngang. All kompilator- og bundlelogikk ligg i Norscode-fila
# tools/compile_with_hybrid_bundle_v9400.no; denne fila mappar berre eldre
# posisjonelle argument til den Norscode-eigde miljøkontrakten.
#
# NORSCODE_HYBRID_ROOT=<repo-root> NORSCODE_HYBRID_SRC=<src.no> NORSCODE_HYBRID_OUT=<out.ncb.json> ./bin/nc run tools/compile_with_hybrid_bundle_v9400.no

if [ "${1:-}" = "--from-no" ]; then
  # Behald eldre kallflate, men aldri vel ein alternativ implementasjon.
  shift
fi

ROOT="${1:-$(pwd)}"
SRC="${2:-}"
OUT="${3:-}"
MODUL="${4:-__main__}"
PROJECT_ROOT="${5:-$ROOT}"
NC_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-$ROOT/dist/norscode_native}"

if [ -z "$SRC" ] || [ -z "$OUT" ]; then
  printf 'argument manglar for Norscode hybrid-kompilering\n' >&2
  exit 1
fi
if [ ! -x "$NC_NATIVE_BIN" ]; then
  printf 'manglar native Norscode-runtime: %s\n' "$NC_NATIVE_BIN" >&2
  exit 1
fi

OUT_DIR=$(dirname "$OUT")
exec env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_HYBRID_ROOT="$ROOT" \
  NORSCODE_HYBRID_SRC="$SRC" \
  NORSCODE_HYBRID_OUT="$OUT" \
  NORSCODE_HYBRID_MODULE="$MODUL" \
  NORSCODE_HYBRID_PROJECT_ROOT="$PROJECT_ROOT" \
  NORSCODE_NATIVE_BIN="$NC_NATIVE_BIN" \
  NORSCODE_VM_CAPABILITIES="${NORSCODE_VM_CAPABILITIES:-env.read,process.exec,disk.read,disk.write}" \
  NORSCODE_VM_DISK_ROOT="${NORSCODE_VM_DISK_ROOT:-$ROOT,$PROJECT_ROOT,$OUT_DIR,.,${TMPDIR:-/tmp},/tmp,/private/tmp}" \
  NORSCODE_CMD=run \
  NORSCODE_FILE="$ROOT/tools/compile_with_hybrid_bundle_v9400.no" \
  "$NC_NATIVE_BIN"
