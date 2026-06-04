#!/bin/sh
# Compatibility wrapper: L6 verify bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec sh "$ROOT/tools/maint/verify_l6.sh" "$@"
