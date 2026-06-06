#!/bin/sh
# Compatibility wrapper: maintainer-bootstrap bur no under tools/maint/.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec sh "$ROOT/tools/maint/bootstrap.sh" "$@"
