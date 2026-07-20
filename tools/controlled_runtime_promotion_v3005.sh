#!/usr/bin/env sh
# Norscode-first wrapper: v3005 kontrollert promotering ligg i tools/controlled_runtime_promotion_v3005.no.
# Shell-delen under mappar CLI-argument til rot/miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
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

if [ "$NORSCODE_PROMOTION_DRY_RUN" = "sann" ]; then
  exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/controlled_runtime_promotion_v3005.no"
fi

# Tung preflight og lett apply/post-gate køyrer i kvar sin Norscode-prosess,
# slik at VM-minnet frå mediaflata er returnert før aktiv runtime blir prøvd.
env NORSCODE_PROMOTION_DRY_RUN=sann NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/controlled_runtime_promotion_v3005.no"
exec env NORSCODE_PROMOTION_PHASE=apply NORSCODE_PROMOTION_DRY_RUN=usann NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/controlled_runtime_promotion_v3005.no"
