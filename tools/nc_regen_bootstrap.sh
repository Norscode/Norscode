#!/bin/sh
# Norscode-first wrapper: bootstrap-regenerering ligg i tools/nc_regen_bootstrap.no.
# Shell-delen under mappar CLI-argument til rot/miljø og startar Norscode-eigarfil.
# bruk: ./bin/nc regen-bootstrap [--full]
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
_full=0
if [ "${1:-}" = "--full" ]; then _full=1; fi
NORSCODE_REGEN_BOOTSTRAP_FULL="$_full" NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/nc_regen_bootstrap.no"
