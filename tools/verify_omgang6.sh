#!/usr/bin/env sh
# Tynn wrapper: Omgang 6-logikken ligg i tools/verify_omgang6.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MODE="${1:-all}"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NC_OM6_MODE="$MODE"

exec "$ROOT/bin/nc" run "$ROOT/tools/verify_omgang6.no"
