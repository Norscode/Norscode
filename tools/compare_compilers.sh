#!/usr/bin/env sh
# Norscode-first wrapper: compiler-samanlikning ligg i tools/compare_compilers.no.
# Shell-delen under mappar CLI-argument/hashar til miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Bruk: NORSCODE_COMPILER_A=<compiler_a> NORSCODE_COMPILER_B=<compiler_b> ./bin/nc run tools/compare_compilers.no"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_COMPILER_A="$1"
export NORSCODE_COMPILER_B="$2"
set -- $(shasum -a 256 "$1")
export NORSCODE_COMPILER_SHA_A="$1"
set -- $(shasum -a 256 "$NORSCODE_COMPILER_B")
export NORSCODE_COMPILER_SHA_B="$1"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/compare_compilers.no"
