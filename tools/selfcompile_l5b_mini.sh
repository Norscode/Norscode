#!/usr/bin/env sh
# Norscode-first wrapper: L5b-mini ligg i tools/selfcompile_l5b_mini.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

OUT="${TMPDIR:-/tmp}/selfcompile_l5b_mini_$$.log"
RC=0

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

"$ROOT/bin/nc" run "$ROOT/tools/selfcompile_l5b_mini.no" >"$OUT" 2>&1 || RC=$?
if [ "$RC" -eq 0 ]; then
  print_file "$OUT"
  rm -f "$OUT"
  exit 0
fi
if ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  rm -f "$OUT"
  exit "$RC"
fi
rm -f "$OUT"

V1="$ROOT/build/l5b/mini_v1.ncb.json"
V2="$ROOT/build/l5b/mini_v2.ncb.json"
mkdir -p "$ROOT/build/l5b"

printf '=== L5b-mini (VM byte-paritet, liten bundle) ===\n\n'
"$ROOT/bin/nc" bundle \
  selfhost.json=selfhost/json.no \
  selfhost.kompiler=selfhost/kompiler.no \
  selfhost.vm=selfhost/vm.no \
  selfhost.bundler=selfhost/bundler.no \
  --output "$V1"

NORSCODE_CMD=l5b-gen2 \
NORSCODE_L5B_V1="$V1" \
NORSCODE_L5B_V2="$V2" \
NORSCODE_BUNDLE_ARGS="selfhost.json=selfhost/json.no selfhost.kompiler=selfhost/kompiler.no selfhost.vm=selfhost/vm.no selfhost.bundler=selfhost/bundler.no" \
  "$ROOT/dist/norscode_native"

if [ ! -f "$V1" ] || [ ! -f "$V2" ]; then
  printf 'FEIL: manglar v1/v2 bundle\n'
  exit 1
fi
if ! cmp -s "$V1" "$V2"; then
  printf 'FEIL: v1 og v2 differ\n'
  exit 1
fi
printf '\n=== L5b-mini: BESTÅTT ===\n'
