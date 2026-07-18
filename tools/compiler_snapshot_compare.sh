#!/usr/bin/env sh
# Norscode-first wrapper: snapshot-samanlikning ligg i tools/compiler_snapshot_compare.no.
# Shell-delen under mappar CLI-argument til miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Bruk: NORSCODE_SNAPSHOT_A=<snapshot_dir_a> NORSCODE_SNAPSHOT_B=<snapshot_dir_b> ./bin/nc run tools/compiler_snapshot_compare.no"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_SNAPSHOT_A="$1"
export NORSCODE_SNAPSHOT_B="$2"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/compiler_snapshot_compare.no"
