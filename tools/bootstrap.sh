#!/bin/sh
# Compatibility wrapper: maintainer-bootstrap bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
printf 'ℹ︎ tools/bootstrap.sh er berre kompat-wrapper. Maintainer-bootstrap bur under tools/maint/.\n' >&2
exec sh "$ROOT/tools/maint/bootstrap.sh" "$@"
