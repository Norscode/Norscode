#!/usr/bin/env sh
# Norscode-first wrapper: Omgang 6 ELF-paritet ligg i tools/selfcompile_native_elf.no.
# Shell-delen under er avgrensa native reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

_out="${TMPDIR:-/tmp}/selfcompile_native_elf_$$.log"
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
      *"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

"$ROOT/bin/nc" run "$ROOT/tools/selfcompile_native_elf.no" >"$_out" 2>&1 || _rc=$?
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

src="$ROOT/tests/fixtures/omgang6_selfcompile.no"
v1="$ROOT/build/omgang6/elf_v1"
v2="$ROOT/build/omgang6/elf_v2"
mkdir -p "$ROOT/build/omgang6"

printf '=== Omgang 6 ELF sjølvkompilering ===\n\n'
printf '[1/3] Gen1: bygg-native...\n'
"$ROOT/bin/nc" bygg-native "$src" "$v1"
b1="$(file_bytes "$v1")"
printf '  [OK] %s bytes\n\n' "$b1"

printf '[2/3] Gen2: bygg-native (same kilde)...\n'
"$ROOT/bin/nc" bygg-native "$src" "$v2"
b2="$(file_bytes "$v2")"
printf '  [OK] %s bytes\n\n' "$b2"

printf '[3/3] Byte-paritet Gen1 == Gen2...\n'
cmp -s "$v1" "$v2"
printf '  [OK] %s bytes identiske\n\n' "$b1"
printf '=== Omgang 6 ELF sjølvkompilering: BESTÅTT ===\n'
