#!/usr/bin/env sh
# tools/verify_seed_only.sh — tynn wrapper for Norscode-native seed-sjekk
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_ROOT="$ROOT" \
  "$ROOT/bin/nc" run "$ROOT/tools/verify_seed_only.no"
