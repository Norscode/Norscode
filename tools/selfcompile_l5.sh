#!/usr/bin/env sh
# Norscode-first wrapper: L5-logikken ligg i selfhost/selfcompile_l5.no.
# Shell-delen under set berre rotmiljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ROOT="$ROOT"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/selfhost/selfcompile_l5.no"
