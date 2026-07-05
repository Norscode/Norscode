#!/usr/bin/env sh
# Tynn wrapper: feature-gate-logikken ligg i tools/norscode_feature_gate.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

NORSCODE_FEATURE_FILES=""
for f in "$@"; do
    NORSCODE_FEATURE_FILES="${NORSCODE_FEATURE_FILES}${f}
"
done

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_FEATURE_FILES

exec "$ROOT/bin/nc" run "$ROOT/tools/norscode_feature_gate.no"
