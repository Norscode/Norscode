#!/usr/bin/env sh
# Tynn wrapper: Omgang 6b.3-orkestrering ligg i tools/selfcompile_stage0_elf.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/selfcompile_stage0_elf.no"
