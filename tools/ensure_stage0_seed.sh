#!/bin/bash
# Compatibility wrapper: stage0-seed-maintenance bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec bash "$ROOT/tools/maint/ensure_stage0_seed.sh" "$@"
