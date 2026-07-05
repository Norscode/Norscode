#!/usr/bin/env sh
# Tynn wrapper: IR-normalisering ligg i tools/ir_canonical_dump.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Usage: ir_canonical_dump.sh <input_ir_dump> <output_ir_dump>"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_IR_INPUT="$1"
export NORSCODE_IR_OUTPUT="$2"

exec "$ROOT/bin/nc" run "$ROOT/tools/ir_canonical_dump.no"
