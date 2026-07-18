#!/usr/bin/env sh
# Norscode-first wrapper: kompatibilitetslogikken ligg i tools/bootstrap_wrapper.no.
# Shell-delen under er avgrensa native reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_BOOTSTRAP_ARG1="${1:-}"

OUT="${TMPDIR:-/tmp}/bootstrap_wrapper_$$.log"
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

"$ROOT/bin/nc" run "$ROOT/tools/bootstrap_wrapper.no" >"$OUT" 2>&1 || RC=$?
if [ "$RC" -eq 0 ]; then
  print_file "$OUT"
  exit 0
fi

if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  exit "$RC"
fi

NATIVE="$ROOT/dist/norscode_native"
if [ ! -x "$NATIVE" ]; then
  printf 'norscode: norscode_native ikkje funnen: %s\n' "$NATIVE"
  printf 'Køyr: ./bin/nc fetch-stage0-seed\n'
  exit 1
fi

if [ -n "${NORCODE_BOOTSTRAP_CLI:-}" ]; then
  CMD="${NORCODE_ARG0:-selfcheck}"
  case "$CMD" in
    selfcheck|test|identity|release|release-targets)
      exec sh "$ROOT/bin/nc" selfcheck
      ;;
    help|--help|-h)
      exec sh "$ROOT/bin/nc" help
      ;;
    *)
      exec sh "$ROOT/bin/nc" "$CMD"
      ;;
  esac
fi

if [ -n "${NORCODE_BOOTSTRAP_VM:-}" ]; then
  CMD2="${NORCODE_CMD:-run}"
  SRC="${NORCODE_SRC:-}"
  DEST="${NORCODE_OUT:-}"
  if [ "$CMD2" = "run" ] && [ -n "$SRC" ]; then
    exec env NORSCODE_CMD=run NORSCODE_FILE="$SRC" "$NATIVE"
  fi
  if [ "$CMD2" = "build" ] && [ -n "$SRC" ] && [ -n "$DEST" ]; then
    exec env NORSCODE_CMD=compile NORSCODE_FILE="$SRC" NORSCODE_OUTPUT="$DEST" "$NATIVE"
  fi
  printf 'norscode: ugyldig NORCODE_CMD=%s\n' "$CMD2"
  exit 1
fi

exec env NORSCODE_CMD=run NORSCODE_FILE="${NORSCODE_BOOTSTRAP_ARG1:-}" "$NATIVE"
