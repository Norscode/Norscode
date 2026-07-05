#!/usr/bin/env sh
# Tynn wrapper: v5400 bootstrap gap check ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/check_bootstrap_gap_v5400.no"
