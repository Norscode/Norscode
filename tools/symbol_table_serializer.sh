#!/usr/bin/env sh
# Norscode-first wrapper: symbolserialisering ligg i tools/symbol_table_serializer.no.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
if [ "$#" -lt 2 ]; then
  echo "Usage: NORSCODE_SYMBOL_INPUT=<input_symbols> NORSCODE_SYMBOL_OUTPUT=<output_symbols> ./bin/nc run tools/symbol_table_serializer.no" >&2
  exit 1
fi
export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_SYMBOL_INPUT="$1"
export NORSCODE_SYMBOL_OUTPUT="$2"
mkdir -p "$(dirname -- "$2")"
exec env NORSCODE_ENABLE_EXEC_PROSESS="$NORSCODE_ENABLE_EXEC_PROSESS" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/symbol_table_serializer.no"
