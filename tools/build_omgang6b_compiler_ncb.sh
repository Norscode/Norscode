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
NORSCODE_USE_PRECOMPILED_SELFHOST=0 \
NORSCODE_BUNDLE_ENTRY='selfhost.elf_compile_driver.start' \
  "$ROOT/bin/nc" bundle \
  selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no \
  selfhost.parser=selfhost/parser.no \
  selfhost.compiler.semantic=selfhost/compiler/semantic.no \
  selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no \
  selfhost.json=selfhost/json.no \
  selfhost.kompiler=selfhost/kompiler.no \
  selfhost.bundler=selfhost/bundler.no \
  selfhost.elf_compile_driver=selfhost/elf_compile_driver.no \
  --output "$NORSCODE_OMGANG6B_NCB_OUT"
mkdir -p "$ROOT/bootstrap/precompiled_fragments" "$ROOT/bootstrap/precompiled_fragments_inner"
"$ROOT/bin/nc" run "$ROOT/tools/regenerate_omgang6b_fragments_safe.no" >/dev/null
printf '✓ Omgang 6b stage-0 NCB: %s (%s bytes)\n' "$NORSCODE_OMGANG6B_NCB_OUT" "$(file_bytes "$NORSCODE_OMGANG6B_NCB_OUT")"
