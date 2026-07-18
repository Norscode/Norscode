#!/usr/bin/env sh
# Norscode-first wrapper: NCB alias-normalisering ligg i tools/ncb_normalize_builtin_aliases_v802.no.
# Shell-delen under mappar CLI-argument til miljø, sikrar utmappe og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ "$#" -ne 2 ]; then
  printf '%s\n' 'Bruk: NORSCODE_NCB_NORMALIZE_IN=<inn.ncb.json> NORSCODE_NCB_NORMALIZE_OUT=<ut.ncb.json> ./bin/nc run tools/ncb_normalize_builtin_aliases_v802.no' >&2
  exit 2
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_NCB_NORMALIZE_IN="$1"
export NORSCODE_NCB_NORMALIZE_OUT="$2"

mkdir -p "$(dirname -- "$2")"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/ncb_normalize_builtin_aliases_v802.no"
