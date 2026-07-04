#!/usr/bin/env bash
# Tynn wrapper: Omgang 6 ELF-paritet ligg i tools/selfcompile_native_elf.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/selfcompile_native_elf.no"
