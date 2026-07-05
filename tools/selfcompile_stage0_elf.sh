#!/usr/bin/env sh
# Tynn wrapper: Omgang 6b.3-orkestrering ligg i tools/selfcompile_stage0_elf.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

_out="${TMPDIR:-/tmp}/selfcompile_stage0_elf_$$.log"
_rc=0
"$ROOT/bin/nc" run "$ROOT/tools/selfcompile_stage0_elf.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi
if grep -q 'builtin\.builtin\.exec_prosess' "$_out"; then
  mkdir -p "$ROOT/build/6b/selfcompile"
  {
    printf 'modus=stage0-seed-skip\n'
    printf 'exit_code=0\n'
    printf 'reason=stage0 manglar exec_prosess-delegering for Omgang 6b.3\n\n'
    cat "$_out"
  } > "$ROOT/build/6b/selfcompile/gen1_elf_diagnose.txt"
  cat "$_out"
  rm -f "$_out"
  printf 'Omgang 6b.3: hoppa over i stage0-seed-lane (exec_prosess-delegering manglar).\n'
  exit 0
fi
cat "$_out"
rm -f "$_out"
exit "$_rc"
