#!/usr/bin/env bash
# Tynn wrapper: NCB→ELF-logikken ligg i tools/ncb_to_elf.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ "$#" -lt 2 ]; then
    printf 'bruk: ncb_to_elf.sh fil.ncb.json ut.elf\n' >&2
    exit 2
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_NCB_TO_ELF_INPUT="$1"
export NORSCODE_NCB_TO_ELF_OUTPUT="$2"

exec "$ROOT/bin/nc" run "$ROOT/tools/ncb_to_elf.no"
