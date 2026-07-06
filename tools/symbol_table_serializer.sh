#!/usr/bin/env sh
# Tynn wrapper: symbolserialisering ligg i tools/symbol_table_serializer.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Usage: symbol_table_serializer.sh <input_symbols> <output_symbols>"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_SYMBOL_INPUT="$1"
export NORSCODE_SYMBOL_OUTPUT="$2"

mkdir -p "$(dirname -- "$2")"

exec "$ROOT/bin/nc" run "$ROOT/tools/symbol_table_serializer.no"
