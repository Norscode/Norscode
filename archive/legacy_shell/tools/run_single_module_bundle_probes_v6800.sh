#!/usr/bin/env sh
# Arkivert overgangs-wrapper. Aktiv eigar er tools/run_single_module_bundle_probes_v6800.no.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
cd "$ROOT"
export NORSCODE_ROOT="$ROOT"
exec "$ROOT/bin/nc" run "$ROOT/tools/run_single_module_bundle_probes_v6800.no"
