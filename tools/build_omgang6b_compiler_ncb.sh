#!/usr/bin/env sh
# Tynn wrapper: Omgang 6b NCB-bygg ligg i tools/build_omgang6b_compiler_ncb.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_OMGANG6B_NCB_OUT="${1:-$ROOT/build/6b/compiler_stage0.ncb.json}"

exec "$ROOT/bin/nc" run "$ROOT/tools/build_omgang6b_compiler_ncb.no"
