#!/usr/bin/env sh
# Norscode-first wrapper: v5400 bootstrap gap-sjekkliste ligg i tools/check_bootstrap_gap_v5400.no.
# Shell-delen under set berre rot/prosessmiljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/check_bootstrap_gap_v5400.no"
