#!/usr/bin/env sh
# Tynn wrapper: v795-testen ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_RUNTIME_GAP_BASE="${NORSCODE_RUNTIME_GAP_BASE:-http://127.0.0.1:8126}"
exec "$ROOT/bin/nc" run "$ROOT/tools/test_v795_runtime_gap.no"
