#!/usr/bin/env sh
# Norscode-first wrapper: Linux stage0 random_hex-probe ligg i tools/probe_linux_stage0_random_hex_v732.no.
# Shell-delen under set berre rot/prosessmiljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/probe_linux_stage0_random_hex_v732.no"
