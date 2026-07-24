#!/usr/bin/env sh
# Norscode-first wrapper: samla lokal grønnliste ligg i tools/local_green.no.
# Shell-delen under er berre avgrensa reserveveg når runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT="${TMPDIR:-/tmp}/local_green_$$.log"
RC=0
STRICT=0
MODE="run"
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
usage() {
  printf 'bruk: ./bin/nc local-green [--strict] [--list|-l|--help|-h]\n'
}
trap cleanup EXIT
trap 'cleanup; exit 129' HUP
trap 'cleanup; exit 130' INT
trap 'cleanup; exit 143' TERM

while [ "$#" -gt 0 ]; do
  case "$1" in
    --strict)
      if [ "$STRICT" = "1" ]; then
        printf 'for mange local-green-val\n' >&2
        usage >&2
        exit 2
      fi
      STRICT=1
      ;;
    --list|-l)
      if [ "$MODE" != "run" ]; then
        printf 'for mange local-green-val\n' >&2
        usage >&2
        exit 2
      fi
      MODE="list"
      ;;
    --help|-h)
      if [ "$MODE" != "run" ]; then
        printf 'for mange local-green-val\n' >&2
        usage >&2
        exit 2
      fi
      MODE="help"
      ;;
    *)
      if [ "$MODE" != "run" ]; then
        printf 'for mange local-green-val\n' >&2
        usage >&2
        exit 2
      fi
      printf 'ukjent local-green-val: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

if [ "$MODE" = "list" ]; then
  env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_LOCAL_GREEN_LIST=1 NORSCODE_LOCAL_GREEN_STRICT="$STRICT" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/local_green.no" >"$OUT" 2>&1 || RC=$?
  if [ "$RC" -eq 0 ]; then
    print_file "$OUT"
    exit 0
  fi
  if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
    print_file "$OUT"
    exit "$RC"
  fi
  printf 'local-green steg (ingen køyring):\n'
  if [ "$STRICT" = "1" ]; then
    printf '  1. Streng release-preflight: ./bin/nc release-preflight --strict\n'
  else
    printf '  1. Release-preflight: ./bin/nc release-preflight\n'
  fi
  printf '  2. Aktiv C/Python-fri flate: ./bin/nc active-surface\n'
  printf '  3. Selfhost fase-0: ./bin/nc phase0-verify\n'
  printf '  4. Sjølvstendighet L1-L6: ./bin/nc selvstendighet\n'
  printf '  5. Full testflate: ./bin/nc test\n'
  exit 0
fi

if [ "$MODE" = "help" ]; then
  RC=0
  env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_LOCAL_GREEN_HELP=1 NORSCODE_LOCAL_GREEN_STRICT="$STRICT" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/local_green.no" >"$OUT" 2>&1 || RC=$?
  if [ "$RC" -eq 0 ]; then
    print_file "$OUT"
    exit 0
  fi
  if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
    print_file "$OUT"
    exit "$RC"
  fi
  usage
  printf '\n'
  printf 'Køyrer lokal grønnliste utan publisering.\n'
  printf 'Bruk --strict for å køyre streng release-preflight som fyrste steg.\n'
  printf 'Bruk --list for å vise stega med kommandoar utan å køyre dei.\n'
  printf 'Bruk --strict --list eller --strict -l for å vise streng grønnliste med kommandoar utan køyring.\n'
  exit 0
fi

env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_LOCAL_GREEN_STRICT="$STRICT" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/local_green.no" >"$OUT" 2>&1 || RC=$?
if [ "$RC" -eq 0 ]; then
  print_file "$OUT"
  exit 0
fi

if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  exit "$RC"
fi

cd "$ROOT"
printf '=== Norscode lokal grønnliste (ingen publisering) ===\n'
if [ "$STRICT" = "1" ]; then
  printf 'Køyrer streng release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate lokalt.\n'
else
  printf 'Køyrer release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate lokalt.\n'
fi

if [ "$STRICT" = "1" ]; then
  printf '\n=== 1. Streng release-preflight ===\n'
  "$ROOT/bin/nc" release-preflight --strict
else
  printf '\n=== 1. Release-preflight ===\n'
  "$ROOT/bin/nc" release-preflight
fi

printf '\n=== 2. Aktiv C/Python-fri flate ===\n'
"$ROOT/bin/nc" active-surface

printf '\n=== 3. Selfhost fase-0 ===\n'
"$ROOT/bin/nc" phase0-verify

printf '\n=== 4. Sjølvstendighet L1-L6 ===\n'
"$ROOT/bin/nc" selvstendighet

printf '\n=== 5. Full testflate ===\n'
"$ROOT/bin/nc" test

printf '\n=== Lokal grønnliste: BESTÅTT ===\n'
