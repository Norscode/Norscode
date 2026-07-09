#!/usr/bin/env sh
# Norscode-first wrapper: semantisk diff ligg i tools/semantic_diff_analyzer.no.
# Shell-delen under mappar CLI-argument til rot/miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Bruk: NORSCODE_SEMANTIC_A=<symbols_a> NORSCODE_SEMANTIC_B=<symbols_b> ./bin/nc run tools/semantic_diff_analyzer.no"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_SEMANTIC_A="$1"
export NORSCODE_SEMANTIC_B="$2"

mkdir -p "$ROOT/reports"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/semantic_diff_analyzer.no"
