#!/usr/bin/env sh
# Tynn wrapper: kompatibilitetslogikken ligg i tools/bootstrap_wrapper.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_BOOTSTRAP_ARG1="${1:-}"

exec "$ROOT/bin/nc" run "$ROOT/tools/bootstrap_wrapper.no"
