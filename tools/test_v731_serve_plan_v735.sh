#!/usr/bin/env sh
# Tynn wrapper: v735-røykprøven ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_V731_PORT="${1:-8126}"
exec "$ROOT/bin/nc" run "$ROOT/tools/test_v731_serve_plan_v735.no"
