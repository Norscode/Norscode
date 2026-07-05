#!/usr/bin/env sh
# Tynn wrapper: v732-proben ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
exec "$ROOT/bin/nc" run "$ROOT/tools/probe_linux_stage0_random_hex_v732.no"
