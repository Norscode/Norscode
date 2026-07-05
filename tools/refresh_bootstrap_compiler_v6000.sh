#!/usr/bin/env sh
# Tynn wrapper: v6000 refresh-løypa ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_V6000_MODE="${1:---dry-run}"
exec "$ROOT/bin/nc" run "$ROOT/tools/refresh_bootstrap_compiler_v6000.no"
