#!/usr/bin/env sh
# Tynn wrapper: snapshot-samanlikninga ligg i tools/compiler_snapshot_compare.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Usage: compiler_snapshot_compare.sh <snapshot_dir_a> <snapshot_dir_b>"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_SNAPSHOT_A="$1"
export NORSCODE_SNAPSHOT_B="$2"

exec "$ROOT/bin/nc" run "$ROOT/tools/compiler_snapshot_compare.no"
