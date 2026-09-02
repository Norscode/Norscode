#!/usr/bin/env sh
# Arkivert overgangs-wrapper. Aktiv eigar er tools/check_real_pipeline_expectations_v5200.no.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
cd "$ROOT"
export NORSCODE_ROOT="$ROOT"
exec "$ROOT/bin/nc" run "$ROOT/tools/check_real_pipeline_expectations_v5200.no"
