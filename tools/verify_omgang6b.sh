#!/usr/bin/env sh
# Tynn wrapper: Omgang 6b-verifisering ligg i tools/verify_omgang6b.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

_out="${TMPDIR:-/tmp}/verify_omgang6b_$$.log"
_rc=0
"$ROOT/bin/nc" run "$ROOT/tools/verify_omgang6b.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi
if grep -q 'builtin\.builtin\.exec_prosess' "$_out"; then
  cat "$_out"
  rm -f "$_out"
  printf 'Omgang 6b: hoppa over i stage0-seed-lane (exec_prosess-delegering manglar).\n'
  exit 0
fi
cat "$_out"
rm -f "$_out"
exit "$_rc"
