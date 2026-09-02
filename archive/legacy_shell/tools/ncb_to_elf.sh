#!/usr/bin/env bash
# Norscode-first wrapper: NCB-til-ELF-logikken ligg i tools/ncb_to_elf.no.
# Shell-delen under er avgrensa native reserveveg når runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ "$#" -lt 2 ]; then
    printf 'bruk: nc ncb-to-elf fil.ncb.json ut.elf\n' >&2
    exit 2
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_NCB_TO_ELF_INPUT="$1"
export NORSCODE_NCB_TO_ELF_OUTPUT="$2"

_out="${TMPDIR:-/tmp}/ncb_to_elf_$$.log"
_rc=0
print_file() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    printf '%s\n' "$_line"
  done < "$_file"
}
has_exec_gap() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}
"$ROOT/bin/nc" run "$ROOT/tools/ncb_to_elf.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  print_file "$_out"
  rm -f "$_out"
  exit 0
fi
if [ -s "$_out" ] && ! has_exec_gap "$_out"; then
  print_file "$_out"
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
