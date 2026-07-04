#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/verify_selvstendighet.no"
