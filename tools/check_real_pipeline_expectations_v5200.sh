#!/usr/bin/env sh
# Tynn wrapper: v5200 real pipeline expectations ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NC_BIN="${NC_BIN:-$ROOT/bin/nc}"

exec "$ROOT/bin/nc" run "$ROOT/tools/check_real_pipeline_expectations_v5200.no"
