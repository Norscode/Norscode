#!/usr/bin/env sh
# Arkivert overgangs-wrapper. Aktiv eigar er tools/unified_bootstrap_verifier.no.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
cd "$ROOT"
export NORSCODE_ROOT="$ROOT"
exec "$ROOT/bin/nc" run "$ROOT/tools/unified_bootstrap_verifier.no"
