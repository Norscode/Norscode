#!/usr/bin/env sh
# Norscode-first wrapper: native-readiness ligg i tools/native_readiness_v3400.no.
# Shell-delen under set berre rot/prosessmiljø/kandidat og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_NATIVE_READINESS_CANDIDATE="${1:-}"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/native_readiness_v3400.no"
