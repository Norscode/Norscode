#!/bin/sh
# Compatibility wrapper for maintainer-only regen.
# Den eigentlege vedlikehaldsbana bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
printf 'ℹ︎ tools/regen_native.sh er berre kompat-wrapper. Bruk helst tools/maint/regen_native.sh direkte.\n' >&2
exec sh "$ROOT/tools/maint/regen_native.sh" "$@"
