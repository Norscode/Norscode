#!/usr/bin/env sh
# Tynn wrapper: NCB alias-normalisering ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ "$#" -ne 2 ]; then
  printf 'bruk: %s <inn.ncb.json> <ut.ncb.json>\n' "$0" >&2
  exit 2
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_NCB_NORMALIZE_IN="$1"
export NORSCODE_NCB_NORMALIZE_OUT="$2"

mkdir -p "$(dirname -- "$2")"

exec "$ROOT/bin/nc" run "$ROOT/tools/ncb_normalize_builtin_aliases_v802.no"
