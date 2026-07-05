#!/usr/bin/env sh
# Tynn wrapper: v704-testen ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_RANDOM_SESSION_PORT="${1:-8118}"
exec "$ROOT/bin/nc" run "$ROOT/tools/test_random_session_v704.no"
