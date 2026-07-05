#!/usr/bin/env sh
# Tynn wrapper: v733 Linux-sjekken ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
exec "$ROOT/bin/nc" run "$ROOT/tools/linux_random_hex_rebuild_check_v733.no"
