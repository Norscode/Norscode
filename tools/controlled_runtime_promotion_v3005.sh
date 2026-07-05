#!/usr/bin/env sh
# Tynn wrapper: v3005 kontrollert promotering ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_PROMOTION_DRY_RUN=usann
export NORSCODE_PROMOTION_CONFIRM=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --dry-run)
      export NORSCODE_PROMOTION_DRY_RUN=sann
      shift
      ;;
    --confirm=*)
      export NORSCODE_PROMOTION_CONFIRM="${1#--confirm=}"
      shift
      ;;
    --confirm)
      shift
      export NORSCODE_PROMOTION_CONFIRM="${1:-}"
      [ "$#" -gt 0 ] && shift
      ;;
    *)
      shift
      ;;
  esac
done

exec "$ROOT/bin/nc" run "$ROOT/tools/controlled_runtime_promotion_v3005.no"
