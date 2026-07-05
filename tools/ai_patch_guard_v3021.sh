#!/usr/bin/env sh
# Tynn wrapper: patch guard ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_PATCH_GUARD_ALLOW_ARCHIVE=usann
export NORSCODE_PATCH_GUARD_MANIFEST=""
export NORSCODE_PATCH_GUARD_PATCH=""

usage() {
  printf 'Bruk: %s [--allow-archive] [--manifest PATH] PATCHFILE\n' "$0" >&2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --allow-archive)
      export NORSCODE_PATCH_GUARD_ALLOW_ARCHIVE=sann
      shift
      ;;
    --manifest)
      [ "$#" -ge 2 ] || { usage; exit 2; }
      export NORSCODE_PATCH_GUARD_MANIFEST="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      if [ -n "$NORSCODE_PATCH_GUARD_PATCH" ]; then
        usage
        exit 2
      fi
      export NORSCODE_PATCH_GUARD_PATCH="$1"
      shift
      ;;
  esac
done

exec "$ROOT/bin/nc" run "$ROOT/tools/ai_patch_guard_v3021.no"
