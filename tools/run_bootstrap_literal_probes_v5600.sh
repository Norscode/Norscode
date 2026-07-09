#!/usr/bin/env sh
# Norscode-first wrapper: v5600 bootstrap literal probes ligg i tools/run_bootstrap_literal_probes_v5600.no.
# Shell-delen under set berre rot/prosessmiljø/binærsti og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export BIN="${BIN:-$ROOT/bin/nc}"
export OUTDIR="${OUTDIR:-$ROOT/build/v5600/out}"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/run_bootstrap_literal_probes_v5600.no"
