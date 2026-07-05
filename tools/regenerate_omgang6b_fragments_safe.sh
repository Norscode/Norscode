#!/usr/bin/env sh
# Tynn wrapper: fragment-regenereringa ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"

_out="${TMPDIR:-/tmp}/regenerate_omgang6b_fragments_$$.log"
_rc=0
"$ROOT/bin/nc" run "$ROOT/tools/regenerate_omgang6b_fragments_safe.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi
if [ -s "$_out" ] && ! grep -Eq 'Ukjent funksjon: builtin(\.builtin)?\.exec_prosess' "$_out"; then
  cat "$_out"
  rm -f "$_out"
  exit "$_rc"
fi
rm -f "$_out"

rewrite_one() {
  src_file="$1"
  module_name="$2"
  src_path="$ROOT/bootstrap/precompiled/$src_file"
  out_name="$(printf '%s' "$src_file" | sed 's/\.ncb\.json$/.functions.json/')"
  outer_path="$ROOT/bootstrap/precompiled_fragments/$out_name"
  inner_path="$ROOT/bootstrap/precompiled_fragments_inner/$out_name"
  jq_filter='.functions | to_entries | map(.key as $old | .value.module = $module | .value.code = (.value.code | map(if (type == "array" and length >= 2 and .[0] == "CALL" and (.[1] | type) == "string" and (.[1] | startswith("__main__."))) then ([.[0], ($module + (.[1][8:]))] + .[2:]) else . end)) | .key = ($module + "." + (if ($old | startswith("__main__.")) then ($old[9:]) else $old end))) | from_entries'
  jq --arg module "$module_name" "$jq_filter" "$src_path" > "$outer_path"
  sed '1s/^{//; $s/}$//' "$outer_path" > "$inner_path"
  printf '%s: outer=%s inner=%s\n' "$out_name" "$(wc -c < "$outer_path" | tr -d ' ')" "$(wc -c < "$inner_path" | tr -d ' ')"
}

mkdir -p "$ROOT/bootstrap/precompiled_fragments" "$ROOT/bootstrap/precompiled_fragments_inner"
rewrite_one lexer_m1.ncb.json selfhost.lexer.lexer_m1
rewrite_one parser.ncb.json selfhost.parser
rewrite_one semantic.ncb.json selfhost.compiler.semantic
rewrite_one ir_to_bytecode.ncb.json selfhost.compiler.ir_to_bytecode
rewrite_one json.ncb.json selfhost.json
rewrite_one kompiler.ncb.json selfhost.kompiler
rewrite_one bundler.ncb.json selfhost.bundler
rewrite_one elf_compile_driver.ncb.json selfhost.elf_compile_driver
