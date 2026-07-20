#!/usr/bin/env sh
# Norscode-first wrapper: Helpdesk API-smoke ligg i tools/smoke_helpdesk_api.no.
# Shell-delen under startar Norscode-eigarfil og har avgrensa test-reserveveg ved manglande prosessbinding.
set -eu
ROOT_DIR="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"
export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
OUT="${TMPDIR:-/tmp}/norscode_smoke_helpdesk_api_$$.log"
RC=0
print_file() { _file="$1"; while IFS= read -r _line || [ -n "$_line" ]; do printf '%s\n' "$_line"; done < "$_file"; }
has_exec_gap() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"Ukjent funksjon: exec_prosess"*|*"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0;;
    esac
  done < "$_file"
  return 1
}
trap 'rm -f "$OUT"' EXIT
"$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/smoke_helpdesk_api.no" >"$OUT" 2>&1 || RC=$?
if [ "$RC" -eq 0 ]; then print_file "$OUT"; exit 0; fi
if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then print_file "$OUT"; exit "$RC"; fi
"$ROOT_DIR/bin/nc" test "$ROOT_DIR/tests/test_helpdesk.no"
