#!/bin/sh
# Norscode-first wrapper: Python-fri CI-flyt ligg i tools/python_free_ci.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

OUT="${TMPDIR:-/tmp}/python_free_ci_$$.log"
RC=0
cleanup() {
  rm -f "$OUT"
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
trap cleanup EXIT

"$ROOT/bin/nc" run "$ROOT/tools/python_free_ci.no" >"$OUT" 2>&1 || RC=$?
if [ "$RC" -eq 0 ]; then
  print_file "$OUT"
  exit 0
fi

if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  exit "$RC"
fi

printf '=== Norscode Python-fri CI ===\n\n'
"$ROOT/bin/nc" active-surface
printf '1. dist/norscode_native...\n'
case "$(uname -s):$(uname -m)" in
  Darwin:arm64) _stage0_platform="macos-arm64" ;;
  Darwin:x86_64) _stage0_platform="macos-x86_64" ;;
  Linux:x86_64|Linux:amd64) _stage0_platform="linux-x86_64" ;;
  Linux:aarch64|Linux:arm64) _stage0_platform="linux-arm64" ;;
  *) _stage0_platform="" ;;
esac
if [ -z "$_stage0_platform" ]; then
  printf 'Feil: ukjent plattform\n'
  exit 1
fi
"$ROOT/bin/nc" fetch-stage0-seed "$_stage0_platform"
printf '\n2. Køyrer testar...\n'
"$ROOT/bin/nc" test
printf '\n=== Python-fri CI: BESTÅTT ===\n'
