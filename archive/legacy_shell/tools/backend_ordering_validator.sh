#!/usr/bin/env sh
# Norscode-first wrapper: backend-rekkefølgjerapport ligg i tools/backend_ordering_validator.no.
# Shell-delen under mappar CLI-argument til miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Bruk: NORSCODE_BACKEND_OUTPUT_A=<backend_output_a> NORSCODE_BACKEND_OUTPUT_B=<backend_output_b> ./bin/nc run tools/backend_ordering_validator.no"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_BACKEND_OUTPUT_A="$1"
export NORSCODE_BACKEND_OUTPUT_B="$2"

mkdir -p "$ROOT/reports"
exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/backend_ordering_validator.no"
