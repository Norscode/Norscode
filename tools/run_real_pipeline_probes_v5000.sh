#!/usr/bin/env sh
# Norscode-first wrapper: real-pipeline probes ligg i tools/run_real_pipeline_probes_v5000.no.
# Shell-delen under set berre rot/prosessmiljø/binærsti og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NC_BIN="${NC_BIN:-$ROOT/bin/nc}"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/run_real_pipeline_probes_v5000.no"
