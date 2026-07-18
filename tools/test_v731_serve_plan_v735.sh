#!/usr/bin/env sh
# Norscode-first wrapper: v735 serve-plan-røykprøven ligg i tools/test_v731_serve_plan_v735.no.
# Shell-delen under set berre rot/prosessmiljø/port og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_V731_PORT="${1:-8126}"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/test_v731_serve_plan_v735.no"
