#!/usr/bin/env sh
# Norscode-first wrapper: native/stage0-promotering ligg i tools/promote_native_stage0_v3001.no.
# Shell-delen under mappar CLI-argument til miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

candidate="${1:-}"
if [ -z "$candidate" ]; then
  printf 'Feil: manglar kandidat-binær\n' >&2
  exit 2
fi
shift || true

promote_dist=0
promote_stage0=0
dry_run=0
for arg in "$@"; do
  case "$arg" in
    --dist) promote_dist=1 ;;
    --stage0) promote_stage0=1 ;;
    --dry-run) dry_run=1 ;;
    *)
      printf 'Feil: ukjent val: %s\n' "$arg" >&2
      exit 2
      ;;
  esac
done

export NORSCODE_ROOT="$ROOT"
export NORSCODE_PROMOTE_CANDIDATE="$candidate"
export NORSCODE_PROMOTE_DIST="$promote_dist"
export NORSCODE_PROMOTE_STAGE0="$promote_stage0"
export NORSCODE_PROMOTE_DRY_RUN="$dry_run"

ARGS_FILE="/tmp/norscode_promote_native_stage0_v3001.env"
{
  printf 'candidate=%s\n' "$candidate"
  printf 'dist=%s\n' "$promote_dist"
  printf 'stage0=%s\n' "$promote_stage0"
  printf 'dry_run=%s\n' "$dry_run"
} > "$ARGS_FILE"

exec env \
  NORSCODE_CMD=run \
  NORSCODE_FILE="$ROOT/tools/promote_native_stage0_v3001.no" \
  NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" \
  NORSCODE_ROOT="$ROOT" \
  NORSCODE_PROMOTE_CANDIDATE="$candidate" \
  NORSCODE_PROMOTE_DIST="$promote_dist" \
  NORSCODE_PROMOTE_STAGE0="$promote_stage0" \
  NORSCODE_PROMOTE_DRY_RUN="$dry_run" \
  NORSCODE_PROMOTE_ARGS_FILE="$ARGS_FILE" \
  "$ROOT/dist/norscode_native"
