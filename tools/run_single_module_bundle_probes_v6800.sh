#!/usr/bin/env sh
# Tynn wrapper: v6800-probene ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
exec "$ROOT/bin/nc" run "$ROOT/tools/run_single_module_bundle_probes_v6800.no"
