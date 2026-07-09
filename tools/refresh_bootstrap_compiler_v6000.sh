#!/usr/bin/env sh
# Norscode-first wrapper: bootstrap-refresh ligg i tools/refresh_bootstrap_compiler_v6000.no.
# Shell-delen under set berre rot/prosessmiljø/modus og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_V6000_MODE="${1:---dry-run}"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/refresh_bootstrap_compiler_v6000.no"
