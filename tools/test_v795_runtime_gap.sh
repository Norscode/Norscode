#!/usr/bin/env sh
# Norscode-first wrapper: v795 runtime-gap-testen ligg i tools/test_v795_runtime_gap.no.
# Shell-delen under set berre rot/prosessmiljø/base-URL og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_RUNTIME_GAP_BASE="${NORSCODE_RUNTIME_GAP_BASE:-http://127.0.0.1:8126}"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/test_v795_runtime_gap.no"
