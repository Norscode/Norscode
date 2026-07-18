#!/usr/bin/env sh
# Norscode-first wrapper: Python-auditen ligg i tools/python_dependency_audit.no.
# Shell-delen under set berre rot/prosessmiljø og startar Norscode-eigarfil.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/python_dependency_audit.no"
