#!/bin/sh
# Compatibility wrapper for maintainer-only regen.
# Den eigentlege vedlikehaldsbana bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec sh "$ROOT/tools/maint/regen_native.sh" "$@"
