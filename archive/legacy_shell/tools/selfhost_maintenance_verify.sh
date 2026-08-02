#!/usr/bin/env sh
# Norscode-first wrapper: vedlikehaldsdocs-policy ligg i tools/selfhost_maintenance_verify.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
if [ -z "${NORSCODE_NATIVE_BIN:-}" ] && [ ! -x "$ROOT_DIR/dist/norscode_native" ]; then
  case "$(uname -s):$(uname -m)" in
    Darwin:arm64) _stage0="$ROOT_DIR/bootstrap/stage0/norscode-macos-arm64" ;;
    Darwin:x86_64) _stage0="$ROOT_DIR/bootstrap/stage0/norscode-macos-x86_64" ;;
    Linux:x86_64) _stage0="$ROOT_DIR/bootstrap/stage0/norscode-linux-x86_64" ;;
    Linux:aarch64|Linux:arm64) _stage0="$ROOT_DIR/bootstrap/stage0/norscode-linux-arm64" ;;
    *) _stage0="" ;;
  esac
  if [ -n "$_stage0" ] && [ -x "$_stage0" ]; then
    export NORSCODE_NATIVE_BIN="$_stage0"
  fi
fi

_out="${TMPDIR:-/tmp}/selfhost_maintenance_verify_$$.log"
_rc=0

print_file() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    printf '%s\n' "$_line"
  done < "$_file"
}

has_expected_missing_text() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"Mangler forventet tekst"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

file_contains_literal() {
  _file="$1"
  _needle="$2"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"$_needle"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

require_literal() {
  _file="$1"
  _needle="$2"
  if ! file_contains_literal "$_file" "$_needle"; then
    printf 'Mangler forventet tekst i %s: %s\n' "$_file" "$_needle"
    exit 1
  fi
}

reject_literal() {
  _file="$1"
  _needle="$2"
  if file_contains_literal "$_file" "$_needle"; then
    printf 'Uønskt tekst i %s: %s\n' "$_file" "$_needle"
    exit 1
  fi
}

env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT_DIR" NORSCODE_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-}" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/selfhost_maintenance_verify.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  print_file "$_out"
  rm -f "$_out"
  exit 0
fi
if ! has_expected_missing_text "$_out"; then
  print_file "$_out"
  rm -f "$_out"
  exit "$_rc"
fi
rm -f "$_out"

require_literal "$ROOT_DIR/docs/INDEX.md" 'docs/_archive/ARCHIVE_INDEX.md'
require_literal "$ROOT_DIR/docs/README.md" 'docs/SELFHOST_HANDLINGSPLAN.md'
require_literal "$ROOT_DIR/docs/05-development/SELFHOST_MIGRATION_AND_DEPRECATIONS.md" 'ARCHIVE_INDEX'
require_literal "$ROOT_DIR/docs/STATUS.md" 'docs/_archive/'
require_literal "$ROOT_DIR/docs/05-development/SELFHOST_CI_GATES.md" 'Normal CI for release/install skal ikkje krevje C-verktøykjede'
require_literal "$ROOT_DIR/docs/05-development/SELFHOST_RELEASE_CHECKLIST.md" 'release/install-flaten krev ikkje C-verktøykjede'
require_literal "$ROOT_DIR/docs/SELFHOST_HANDLINGSPLAN.md" 'C-regen skal berre finnast i eksplisitt vedlikehaldslane'
reject_literal "$ROOT_DIR/docs/STATUS.md" 'Neste konkrete patch'
printf 'Vedlikeholdsverifikasjon: OK\n'
