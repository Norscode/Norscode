#!/usr/bin/env sh
# Tynn wrapper: arkivert v3002-kandidatmelding ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/build_native_candidate_v3002.no"
