#!/usr/bin/env sh
# Norscode-first wrapper: Omgang 6b NCB-bygg ligg i tools/build_omgang6b_compiler_ncb.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_OMGANG6B_NCB_OUT="${1:-${NORSCODE_OMGANG6B_NCB_OUT:-$ROOT/build/6b/compiler_stage0.ncb.json}}"

_out="${TMPDIR:-/tmp}/build_omgang6b_compiler_ncb_$$.log"
_rc=0

first_word() {
  _text="$1"
  set -- $_text
  printf '%s' "${1:-}"
}

file_bytes() {
  _file="$1"
  first_word "$(wc -c "$_file")"
}

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
      *"Ukjent funksjon: exec_prosess"*|*"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

"$ROOT/bin/nc" run "$ROOT/tools/build_omgang6b_compiler_ncb.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  print_file "$_out"
  rm -f "$_out"
  exit 0
fi
if ! has_exec_gap "$_out"; then
  print_file "$_out"
  rm -f "$_out"
  exit "$_rc"
fi
rm -f "$_out"

mkdir -p "$(dirname "$NORSCODE_OMGANG6B_NCB_OUT")"
mkdir -p "$ROOT/bootstrap/precompiled_fragments" "$ROOT/bootstrap/precompiled_fragments_inner"
if [ "${NC_REGEN_OMGANG6B_FRAGMENTS:-0}" = "1" ]; then
  "$ROOT/bin/nc" run "$ROOT/tools/regenerate_omgang6b_fragments_safe.no" >/dev/null
fi
{
  printf '{"format":"ncb-v1","entry":"selfhost.elf_compile_driver.start","imports":[],"route_handlers":{},"dependency_providers":{},"guard_providers":{},"request_middlewares":[],"response_middlewares":[],"error_middlewares":[],"startup_hooks":[],"shutdown_hooks":[],"tests":{},"functions":{'
  _sep=""
  for _frag in lexer_m1 parser semantic ir_to_bytecode json kompiler bundler elf_compile_driver; do
    printf '%s' "$_sep"
    _file="$ROOT/bootstrap/precompiled_fragments_inner/$_frag.functions.json"
    while IFS= read -r _line || [ -n "$_line" ]; do
      printf '%s' "$_line"
    done < "$_file"
    _sep=","
  done
  printf '}}'
} > "$NORSCODE_OMGANG6B_NCB_OUT"
printf '✓ Omgang 6b stage-0 NCB: %s (%s bytes)\n' "$NORSCODE_OMGANG6B_NCB_OUT" "$(file_bytes "$NORSCODE_OMGANG6B_NCB_OUT")"
