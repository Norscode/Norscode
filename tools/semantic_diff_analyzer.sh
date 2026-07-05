#!/usr/bin/env sh
# Tynn wrapper: semantisk diff ligg i tools/semantic_diff_analyzer.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Usage: semantic_diff_analyzer.sh <symbols_a> <symbols_b>"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_SEMANTIC_A="$1"
export NORSCODE_SEMANTIC_B="$2"

exec "$ROOT/bin/nc" run "$ROOT/tools/semantic_diff_analyzer.no"
