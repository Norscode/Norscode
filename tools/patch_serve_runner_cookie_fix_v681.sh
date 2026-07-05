#!/usr/bin/env sh
# Tynn wrapper: arkivert cookie-hotfixmelding ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/patch_serve_runner_cookie_fix_v681.no"
