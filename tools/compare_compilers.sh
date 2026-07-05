#!/usr/bin/env sh
# Tynn wrapper: samanlikninga ligg i tools/compare_compilers.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Usage: compare_compilers.sh <compiler_a> <compiler_b>"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_COMPILER_A="$1"
export NORSCODE_COMPILER_B="$2"

exec "$ROOT/bin/nc" run "$ROOT/tools/compare_compilers.no"
