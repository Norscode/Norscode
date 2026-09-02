#!/usr/bin/env sh
# Arkivert overgangs-wrapper. Aktiv eigar er tools/compare_compilers.no.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
cd "$ROOT"
export NORSCODE_ROOT="$ROOT"
exec "$ROOT/bin/nc" run "$ROOT/tools/compare_compilers.no"
