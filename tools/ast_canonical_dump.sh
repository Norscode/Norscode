#!/usr/bin/env sh
# Tynn wrapper: AST-normalisering ligg i tools/ast_canonical_dump.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Usage: ast_canonical_dump.sh <input_ast_dump> <output_dump>"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_AST_INPUT="$1"
export NORSCODE_AST_OUTPUT="$2"

exec "$ROOT/bin/nc" run "$ROOT/tools/ast_canonical_dump.no"
