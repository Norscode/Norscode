#!/usr/bin/env sh
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
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT_DIR" NORSCODE_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-}" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/selfhost_maintenance_verify.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi
if ! grep -q 'Mangler forventet tekst' "$_out"; then
  cat "$_out"
  rm -f "$_out"
  exit "$_rc"
fi
rm -f "$_out"

grep -F -q -- 'docs/_archive/ARCHIVE_INDEX.md' "$ROOT_DIR/docs/INDEX.md"
grep -F -q -- 'docs/SELFHOST_HANDLINGSPLAN.md' "$ROOT_DIR/docs/README.md"
grep -F -q -- 'ARCHIVE_INDEX' "$ROOT_DIR/docs/05-development/SELFHOST_MIGRATION_AND_DEPRECATIONS.md"
grep -F -q -- 'docs/_archive/' "$ROOT_DIR/docs/STATUS.md"
grep -F -q -- 'Normal CI for release/install skal ikkje krevje C-verktøykjede' "$ROOT_DIR/docs/05-development/SELFHOST_CI_GATES.md"
grep -F -q -- 'release/install-flaten krever ikke C-verktøykjede' "$ROOT_DIR/docs/05-development/SELFHOST_RELEASE_CHECKLIST.md"
grep -F -q -- 'C-regen skal berre finnast i eksplisitt vedlikehaldslane' "$ROOT_DIR/docs/SELFHOST_HANDLINGSPLAN.md"
if grep -F -q -- 'Neste konkrete patch' "$ROOT_DIR/docs/STATUS.md"; then
  printf 'Uønskt tekst i %s: Neste konkrete patch\n' "$ROOT_DIR/docs/STATUS.md"
  exit 1
fi
printf 'Vedlikeholdsverifikasjon: OK\n'
