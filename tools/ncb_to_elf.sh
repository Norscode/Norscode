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

_out="${TMPDIR:-/tmp}/ncb_to_elf_$$.log"
_rc=0
"$ROOT/bin/nc" run "$ROOT/tools/ncb_to_elf.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi
if [ -s "$_out" ] && ! grep -Eq 'Ukjent funksjon: builtin(\.builtin)?\.exec_prosess' "$_out"; then
  cat "$_out"
  rm -f "$_out"
  exit "$_rc"
fi
rm -f "$_out"

env \
  NC_INPUT="$NORSCODE_NCB_TO_ELF_INPUT" \
  NC_OUTPUT="$NORSCODE_NCB_TO_ELF_OUTPUT" \
  NORSCODE_CMD=run \
  NORSCODE_FILE="$ROOT/selfhost/native_execution/ncb_to_elf.no" \
  "$ROOT/dist/norscode_native"
chmod +x "$NORSCODE_NCB_TO_ELF_OUTPUT" 2>/dev/null || true
