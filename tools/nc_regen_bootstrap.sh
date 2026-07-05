#!/usr/bin/env sh
# Tynn wrapper: bootstrap-regenerering ligg i tools/nc_regen_bootstrap.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

FULL="0"
case "${1:-}" in
  --full)
    FULL="1"
    shift
    ;;
  -h|--help)
    printf 'bruk: sh tools/nc_regen_bootstrap.sh [--full]\n'
    exit 0
    ;;
esac

if [ "$#" -gt 0 ]; then
  printf 'bruk: sh tools/nc_regen_bootstrap.sh [--full]\n' >&2
  exit 2
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_REGEN_BOOTSTRAP_FULL="$FULL"

exec "$ROOT/bin/nc" run "$ROOT/tools/nc_regen_bootstrap.no"
