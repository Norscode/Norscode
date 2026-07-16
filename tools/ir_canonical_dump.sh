#!/usr/bin/env sh
# Norscode-first wrapper: IR-normalisering ligg i tools/ir_canonical_dump.no.
# Shell-delen under mappar CLI-argument til miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Bruk: NORSCODE_IR_INPUT=<input_ir_dump> NORSCODE_IR_OUTPUT=<output_ir_dump> ./bin/nc run tools/ir_canonical_dump.no"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="\${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_IR_INPUT="$1"
export NORSCODE_IR_OUTPUT="$2"

mkdir -p "$(dirname -- "$2")"
NORSCODE_ENABLE_EXEC_PROSESS="\${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/ir_canonical_dump.no"
