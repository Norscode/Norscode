#!/usr/bin/env sh
set -eu

# Norscode-first wrapper: eigarlogikken ligg i bundle_with_hybrid_compiler_v9500.no.
# Kompatibilitetsinngang. All bundlelogikk ligg i Norscode-fila
# tools/bundle_with_hybrid_compiler_v9500.no; denne fila mappar berre eldre
# posisjonelle argument til den Norscode-eigde miljøkontrakten.
# NORSCODE_HYBRID_ROOT=<repo-root> NORSCODE_HYBRID_OUT=<out.ncb.json> NORSCODE_HYBRID_ENTRY=<entry> NORSCODE_HYBRID_SPECS='<alias=fil.no>...' ./bin/nc run tools/bundle_with_hybrid_compiler_v9500.no

if [ "${1:-}" = "--from-no" ]; then
  # Behald eldre kallflate, men aldri vel ein alternativ implementasjon.
  shift
fi

ROOT="${1:-}"
OUT="${2:-}"
ENTRY="${3:-}"
shift 3 2>/dev/null || true

if [ -z "$ROOT" ] || [ -z "$OUT" ]; then
  printf 'argument manglar for Norscode hybrid-bundling\n' >&2
  exit 2
fi

SPECS=""
for spec in "$@"; do
  if [ -z "$SPECS" ]; then
    SPECS="$spec"
  else
    SPECS="$SPECS
$spec"
  fi
done

NC_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-$ROOT/dist/norscode_native}"
if [ ! -x "$NC_NATIVE_BIN" ]; then
  printf 'manglar native Norscode-runtime: %s\n' "$NC_NATIVE_BIN" >&2
  exit 1
fi

OUT_DIR=$(dirname "$OUT")
exec env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_HYBRID_ROOT="$ROOT" \
  NORSCODE_HYBRID_OUT="$OUT" \
  NORSCODE_HYBRID_ENTRY="$ENTRY" \
  NORSCODE_HYBRID_SPECS="$SPECS" \
  NORSCODE_NATIVE_BIN="$NC_NATIVE_BIN" \
  NORSCODE_VM_CAPABILITIES="${NORSCODE_VM_CAPABILITIES:-env.read,process.exec,disk.read,disk.write}" \
  NORSCODE_VM_DISK_ROOT="${NORSCODE_VM_DISK_ROOT:-$ROOT,$OUT_DIR,.,${TMPDIR:-/tmp},/tmp,/private/tmp}" \
  NORSCODE_CMD=run \
  NORSCODE_FILE="$ROOT/tools/bundle_with_hybrid_compiler_v9500.no" \
  "$NC_NATIVE_BIN"
