#!/usr/bin/env sh
# Tynn wrapper: Helpdesk HTTP-smoke ligg i tools/smoke_helpdesk_http.no.

set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_HELPDESK_BASE_URL="${1:-http://127.0.0.1:4173}"

exec "$ROOT/bin/nc" run "$ROOT/tools/smoke_helpdesk_http.no"
