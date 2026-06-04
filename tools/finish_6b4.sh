#!/bin/sh
# Compatibility wrapper: finish-6b4 bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec sh "$ROOT/tools/maint/finish_6b4.sh" "$@"
