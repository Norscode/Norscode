#!/usr/bin/env sh
# Norscode-first wrapper: AI patch-vakta ligg i tools/ai_patch_guard_v3021.no.
# Shell-delen under mappar CLI-argument til rot/miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_PATCH_GUARD_ALLOW_ARCHIVE=usann
export NORSCODE_PATCH_GUARD_MANIFEST=""
export NORSCODE_PATCH_GUARD_PATCH=""

usage() {
  printf 'Bruk: NORSCODE_PATCH_GUARD_PATCH=PATCHFILE ./bin/nc run tools/ai_patch_guard_v3021.no\n' >&2
  printf 'Valfritt: set NORSCODE_PATCH_GUARD_ALLOW_ARCHIVE=sann og NORSCODE_PATCH_GUARD_MANIFEST=PATH\n' >&2
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

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/ai_patch_guard_v3021.no"
