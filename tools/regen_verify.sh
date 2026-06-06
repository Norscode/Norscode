#!/bin/sh
# Compatibility wrapper: L6 regen-verify bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec sh "$ROOT/tools/maint/regen_verify.sh" "$@"
