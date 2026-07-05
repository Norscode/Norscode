#!/usr/bin/env sh
# Tynn wrapper: L5-logikken ligg i selfhost/selfcompile_l5.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/selfhost/selfcompile_l5.no"
