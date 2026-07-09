#!/usr/bin/env sh
# Norscode-first wrapper: Helpdesk HTTP-smoke ligg i tools/smoke_helpdesk_http.no.
# Shell-delen under set berre rot/prosessmiljø/base-URL og startar Norscode-eigarfil.

set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_HELPDESK_BASE_URL="${1:-http://127.0.0.1:4173}"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/smoke_helpdesk_http.no"
