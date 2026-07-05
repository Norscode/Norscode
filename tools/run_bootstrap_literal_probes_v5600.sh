#!/usr/bin/env sh
# Tynn wrapper: v5600 bootstrap literal probes ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export BIN="${BIN:-$ROOT/bin/nc}"
export OUTDIR="${OUTDIR:-$ROOT/build/v5600/out}"

exec "$ROOT/bin/nc" run "$ROOT/tools/run_bootstrap_literal_probes_v5600.no"
