#!/usr/bin/env sh
# Tynn wrapper: rapportlogikken ligg i tools/backend_ordering_validator.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ $# -lt 2 ]; then
    echo "Usage: backend_ordering_validator.sh <backend_output_a> <backend_output_b>"
    exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_BACKEND_OUTPUT_A="$1"
export NORSCODE_BACKEND_OUTPUT_B="$2"

exec "$ROOT/bin/nc" run "$ROOT/tools/backend_ordering_validator.no"
