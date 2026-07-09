#!/usr/bin/env sh
# Norscode-first wrapper: AST-normalisering ligg i tools/ast_canonical_dump.no.
# Shell-delen under mappar CLI-argument til miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Bruk: NORSCODE_AST_INPUT=<input_ast_dump> NORSCODE_AST_OUTPUT=<output_dump> ./bin/nc run tools/ast_canonical_dump.no"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_AST_INPUT="$1"
export NORSCODE_AST_OUTPUT="$2"

mkdir -p "$(dirname -- "$2")"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/ast_canonical_dump.no"
