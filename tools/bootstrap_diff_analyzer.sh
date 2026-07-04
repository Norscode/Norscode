#!/usr/bin/env sh
# Tynn wrapper: bootstrap-diff ligg i tools/bootstrap_diff_analyzer.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Usage: bootstrap_diff_analyzer.sh <build_a> <build_b>"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_BUILD_A="$1"
export NORSCODE_BUILD_B="$2"

exec "$ROOT/bin/nc" run "$ROOT/tools/bootstrap_diff_analyzer.no"
