#!/usr/bin/env bash
# Arkivert overgangs-wrapper. Aktiv eigar er tools/generate_build_embed_c.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
cd "$ROOT"
export NORSCODE_ROOT="$ROOT"
exec "$ROOT/bin/nc" run "$ROOT/tools/generate_build_embed_c.no"
