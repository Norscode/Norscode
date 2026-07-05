#!/usr/bin/env sh
# Tynn wrapper: v3005 Linux-kandidatbygg ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
exec "$ROOT/bin/nc" run "$ROOT/tools/build_linux_native_candidate_v3005.no"
