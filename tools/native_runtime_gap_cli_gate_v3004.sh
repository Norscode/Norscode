#!/usr/bin/env sh
# Tynn wrapper: CLI runtime-gap gate ligg i tools/native_runtime_gap_cli_gate_v3004.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/native_runtime_gap_cli_gate_v3004.no"
