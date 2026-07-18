#!/usr/bin/env sh
# Norscode-first wrapper: v704 random-session-testen ligg i tools/test_random_session_v704.no.
# Shell-delen under set berre rot/prosessmiljø/port og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_RANDOM_SESSION_PORT="${1:-8118}"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/test_random_session_v704.no"
