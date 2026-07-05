#!/usr/bin/env sh
# Tynn wrapper: native runtime-gap gate ligg i tools/native_runtime_gap_gate_v3001.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_NATIVE_GAP_BIN="${1:-$ROOT/dist/norscode_native}"

exec "$ROOT/bin/nc" run "$ROOT/tools/native_runtime_gap_gate_v3001.no"
