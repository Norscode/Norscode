#!/usr/bin/env sh
set -eu
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT_DIR" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/selfhost_maintenance_verify.no"
