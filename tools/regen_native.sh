#!/bin/sh
# Compatibility wrapper: C-regen bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec sh "$ROOT/tools/maint/regen_native.sh" "$@"
