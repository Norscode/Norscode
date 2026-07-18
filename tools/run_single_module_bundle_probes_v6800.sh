#!/usr/bin/env sh
# Norscode-first wrapper: v6800 single-module bundle-probene ligg i tools/run_single_module_bundle_probes_v6800.no.
# Shell-delen under set berre rot/prosessmiljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/run_single_module_bundle_probes_v6800.no"
