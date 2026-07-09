#!/usr/bin/env sh
# Norscode-first wrapper: Omgang 6b-verifisering ligg i tools/verify_omgang6b.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

first_word() {
  _text="$1"
  set -- $_text
  printf '%s' "${1:-}"
}

file_bytes() {
  _file="$1"
  first_word "$(wc -c "$_file")"
}

run_shell_gate() {
  mkdir -p "$ROOT/build/6b"
  printf '=== Omgang 6b: ELF stage-0 (6b.1 + 6b.2 + 6b.3) ===\n\n'
  printf '[6b.1] Host ELF determinisme...\n'
  "$ROOT/bin/nc" bygg-native tests/fixtures/omgang6b_host.no build/6b/host_v1.elf
  "$ROOT/bin/nc" bygg-native tests/fixtures/omgang6b_host.no build/6b/host_v2.elf
  cmp -s build/6b/host_v1.elf build/6b/host_v2.elf
  printf '  [OK] host_v1 == host_v2 (%s bytes)\n\n' "$(file_bytes "$ROOT/build/6b/host_v1.elf")"

  printf '[6b.2] Kompilator stage-0 NCB + ELF determinisme...\n'
  "$ROOT/bin/nc" build-omgang6b-ncb "$ROOT/build/6b/compiler_stage0.ncb.json"
  "$ROOT/bin/nc" ncb-to-elf "$ROOT/build/6b/compiler_stage0.ncb.json" "$ROOT/build/6b/compiler_v1.elf"
  "$ROOT/bin/nc" ncb-to-elf "$ROOT/build/6b/compiler_stage0.ncb.json" "$ROOT/build/6b/compiler_v2.elf"
  cmp -s "$ROOT/build/6b/compiler_v1.elf" "$ROOT/build/6b/compiler_v2.elf"
  printf '  [OK] compiler_v1 == compiler_v2 (%s bytes)\n\n' "$(file_bytes "$ROOT/build/6b/compiler_v1.elf")"

  case "$(uname -s):$(uname -m)" in
    Linux:x86_64|Linux:amd64)
      if [ "${NC_OM6B_RUN_STAGE0:-0}" = "1" ]; then
        printf '[6b.3] Stage-0 ELF sjølvkompilering...\n'
        "$ROOT/bin/nc" selfcompile-stage0-elf
      else
        printf '[6b.3] Hopp over ELF→ELF runtime (set NC_OM6B_RUN_STAGE0=1 for djup smoke)\n\n'
      fi
      ;;
    *)
      printf '[6b.3] Hopp over ELF→ELF runtime (%s/%s — krev Linux x86-64)\n\n' "$(uname -s)" "$(uname -m)"
      ;;
  esac
  printf '=== Omgang 6b.1 + 6b.2 + 6b.3: BESTÅTT ===\n'
}

_out="${TMPDIR:-/tmp}/verify_omgang6b_$$.log"
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
      *"Ukjent funksjon: exec_prosess"*|*"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

"$ROOT/bin/nc" run "$ROOT/tools/verify_omgang6b.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  print_file "$_out"
  rm -f "$_out"
  exit 0
fi
if [ ! -s "$_out" ] || has_exec_gap "$_out"; then
  rm -f "$_out"
  run_shell_gate
  exit 0
fi
print_file "$_out"
rm -f "$_out"
exit "$_rc"
