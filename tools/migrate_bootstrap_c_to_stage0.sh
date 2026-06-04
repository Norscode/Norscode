#!/bin/sh
# Compatibility wrapper: bootstrap-c-migrering bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec sh "$ROOT/tools/maint/migrate_bootstrap_c_to_stage0.sh" "$@"
