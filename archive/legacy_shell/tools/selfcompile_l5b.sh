#!/usr/bin/env sh
# Norscode-first wrapper: L5b-orkestrering ligg i tools/selfcompile_l5b.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

OUT="${TMPDIR:-/tmp}/selfcompile_l5b_$$.log"
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

file_bytes() {
  _file="$1"
  if [ -f "$_file" ]; then
    wc -c "$_file" | {
      read -r _bytes _rest
      printf '%s' "$_bytes"
    }
  else
    printf '0'
  fi
}

BUNDLE_ARGS="selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no selfhost.parser=selfhost/parser.no selfhost.compiler.semantic=selfhost/compiler/semantic.no selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no selfhost.kompiler=selfhost/kompiler.no selfhost.json=selfhost/json.no selfhost.vm=selfhost/vm.no selfhost.bundler=selfhost/bundler.no"
V1="$ROOT/build/l5b/compiler_v1.ncb.json"
V2="$ROOT/build/l5b/compiler_v2.ncb.json"

"$ROOT/bin/nc" run "$ROOT/tools/selfcompile_l5b.no" >"$OUT" 2>&1 || RC=$?
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

if [ ! -x "$ROOT/dist/norscode_native" ]; then
  printf 'Trenger dist/norscode_native. Køyr: ./bin/nc fetch-stage0-seed\n'
  exit 1
fi

mkdir -p "$ROOT/build/l5b"
printf '=== L5b: VM sjølvkompilering (byte-paritet) ===\n\n'
printf '[1/3] Gen1: kompilator-bundle (bootstrap-host)...\n'
"$ROOT/bin/nc" bundle \
  selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no \
  selfhost.parser=selfhost/parser.no \
  selfhost.compiler.semantic=selfhost/compiler/semantic.no \
  selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no \
  selfhost.kompiler=selfhost/kompiler.no \
  selfhost.json=selfhost/json.no \
  selfhost.vm=selfhost/vm.no \
  selfhost.bundler=selfhost/bundler.no \
  --output "$V1"
printf '  [OK] %s bytes\n\n' "$(file_bytes "$V1")"

printf '[2/3] Gen2: bundle via Gen1-bytekode (bygg_bundle)...\n'
NORSCODE_CMD=l5b-gen2 \
NORSCODE_L5B_V1="$V1" \
NORSCODE_L5B_V2="$V2" \
NORSCODE_BUNDLE_ARGS="$BUNDLE_ARGS" \
  "$ROOT/dist/norscode_native"

if [ ! -f "$V2" ]; then
  printf '  [FEIL] Gen2 skreiv ikkje %s\n' "$V2"
  exit 1
fi
printf '\n[3/3] Byte-paritet Gen1 == Gen2...\n'
if cmp -s "$V1" "$V2"; then
  printf '  [OK] %s bytes identiske\n\n' "$(file_bytes "$V1")"
  printf '=== L5b: BESTÅTT ===\n'
  printf 'Kompilator-bundle frå VM er byte-identisk med bootstrap-host.\n'
  exit 0
fi
printf '  [FEIL] Bundles differ (v1=%s, v2=%s bytes)\n' "$(file_bytes "$V1")" "$(file_bytes "$V2")"
exit 1
