#!/usr/bin/env sh
# Tynn wrapper: v3007 native-status ligg i tools/native_active_status_v3007.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_NATIVE_STATUS_CANDIDATE="${1:-}"

exec "$ROOT/bin/nc" run "$ROOT/tools/native_active_status_v3007.no"
