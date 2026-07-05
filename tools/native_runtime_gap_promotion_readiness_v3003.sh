#!/usr/bin/env sh
# Tynn wrapper: promotion-readiness ligg i tools/native_runtime_gap_promotion_readiness_v3003.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/native_runtime_gap_promotion_readiness_v3003.no"
