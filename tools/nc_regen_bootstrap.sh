#!/usr/bin/env sh
# Norscode-first wrapper: bootstrap-regenerering ligg i tools/nc_regen_bootstrap.no.
# Shell-delen under mappar CLI-argument til rot/miljø og startar Norscode-eigarfil.
# Shell-delen under er berre avgrensa reserveveg når runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
OUT="${TMPDIR:-/tmp}/nc_regen_bootstrap_$$.log"
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
normalize_ncb() {
  _ncb="$1"
  env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" \
    NORSCODE_ROOT="$ROOT" \
    NORSCODE_NCB_NORMALIZE_IN="$_ncb" \
    NORSCODE_NCB_NORMALIZE_OUT="$_ncb" \
    "$ROOT/bin/nc" run "$ROOT/tools/ncb_normalize_builtin_aliases_v802.no" >/dev/null
}
fallback_regen() {
  if [ "$FULL" = "1" ]; then
    printf 'Genererer full bootstrap/kompiler.ncb.json...\n'
    _tmp="$(mktemp "${TMPDIR:-/tmp}/nc_bundle_XXXXXX.ncb.json")"
    NORSCODE_USE_PRECOMPILED_SELFHOST=0 "$ROOT/bin/nc" bundle \
      selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no \
      selfhost.parser=selfhost/parser.no \
      selfhost.compiler.semantic=selfhost/compiler/semantic.no \
      selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no \
      selfhost.kompiler=selfhost/kompiler.no \
      selfhost.json=selfhost/json.no \
      selfhost.vm=selfhost/vm.no \
      selfhost.bundler=selfhost/bundler.no \
      selfhost.nc_main=selfhost/nc_main.no \
      --output "$_tmp"
    normalize_ncb "$_tmp"
    cp "$_tmp" "$ROOT/bootstrap/kompiler.ncb.json"
    rm -f "$_tmp"
    printf '✓ bootstrap/kompiler.ncb.json\n'
  fi

  printf 'Oppdaterer VM-precompiled NCB...\n'
  "$ROOT/bin/nc" compile selfhost/vm.no bootstrap/precompiled/vm.ncb.json >/dev/null
  normalize_ncb "$ROOT/bootstrap/precompiled/vm.ncb.json"
  printf '✓ bootstrap/precompiled/vm.ncb.json\n'

  printf 'Oppdaterer stdlib-NCBs...\n'
  mkdir -p "$ROOT/bootstrap/stdlib"
  _ok=0
  for _mod in \
    std/math.no std/tekst.no std/liste.no std/ordbok.no std/json.no std/feil.no \
    std/env.no std/io.no std/fil.no std/log.no std/metrics.no std/path.no \
    std/cache.no std/stil.no selfhost/common.no selfhost/ir_contract.no \
    selfhost/compiler.no selfhost/compiler_bridge.no
  do
    if [ -f "$ROOT/$_mod" ]; then
      _name="$_mod"
      while :; do
        case "$_name" in
          */*) _name="${_name%%/*}_${_name#*/}" ;;
          *) break ;;
        esac
      done
      _name="${_name%.no}.ncb.json"
      if "$ROOT/bin/nc" compile "$_mod" "bootstrap/stdlib/$_name" >/dev/null 2>&1; then
        _ok=$((_ok + 1))
      fi
    fi
  done
  printf '✓ %s stdlib-NCBs oppdatert\n' "$_ok"

  printf 'Oppdaterer stilark-precompiled NCB...\n'
  "$ROOT/bin/nc" compile std/stil.no bootstrap/precompiled/stil.ncb.json >/dev/null
  printf '✓ bootstrap/precompiled/stil.ncb.json\n'

  printf 'Røyktesting...\n'
  _smoke="${TMPDIR:-/tmp}/_nc_regen_smoke.no"
  printf 'funksjon start() { skriv("regen OK") }\n' > "$_smoke"
  _smoke_out="$("$ROOT/bin/nc" run "$_smoke" 2>/dev/null || true)"
  rm -f "$_smoke"
  if [ "$_smoke_out" = "regen OK" ]; then
    printf '✓ Røyktest bestått\n\n'
    printf '✓ Bootstrap regenerert!\n'
    return 0
  fi
  printf '✗ Røyktest feilet: %s\n' "$_smoke_out" >&2
  return 1
}
trap cleanup EXIT
trap 'cleanup; exit 129' HUP
trap 'cleanup; exit 130' INT
trap 'cleanup; exit 143' TERM

FULL="0"
case "${1:-}" in
  --full)
    FULL="1"
    shift
    ;;
  -h|--help)
    printf 'bruk: ./bin/nc regen-bootstrap [--full]\n'
    exit 0
    ;;
esac

if [ "$#" -gt 0 ]; then
  printf 'bruk: ./bin/nc regen-bootstrap [--full]\n' >&2
  exit 2
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_REGEN_BOOTSTRAP_FULL="$FULL"

RC=0
env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/nc_regen_bootstrap.no" >"$OUT" 2>&1 || RC=$?
if [ "$RC" -eq 0 ]; then
  print_file "$OUT"
  exit 0
fi

if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  exit "$RC"
fi

fallback_regen
