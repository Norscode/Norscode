#!/usr/bin/env sh
# Norscode-first wrapper: v5600 literal expectation gate ligg i tools/check_bootstrap_literal_expectations_v5600.no.
# Shell-delen under set berre rot/prosessmiljø/outputmappe og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export OUTDIR="${OUTDIR:-$ROOT/build/v5600/out}"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/check_bootstrap_literal_expectations_v5600.no"
