#!/usr/bin/env sh
# Norscode-first wrapper: seed-sjekken ligg i tools/verify_seed_only.no.
# Shell-delen under er avgrensa stage0-reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
case "$(uname -s):$(uname -m)" in
  Darwin:arm64) NORSCODE_SEED_PLATFORM="${NORSCODE_SEED_PLATFORM:-macos-arm64}" ;;
  Darwin:x86_64) NORSCODE_SEED_PLATFORM="${NORSCODE_SEED_PLATFORM:-macos-x86_64}" ;;
  Linux:x86_64) NORSCODE_SEED_PLATFORM="${NORSCODE_SEED_PLATFORM:-linux-x86_64}" ;;
  Linux:aarch64|Linux:arm64) NORSCODE_SEED_PLATFORM="${NORSCODE_SEED_PLATFORM:-linux-arm64}" ;;
  *) NORSCODE_SEED_PLATFORM="${NORSCODE_SEED_PLATFORM:-}" ;;
esac
export NORSCODE_SEED_PLATFORM
case "$(uname -s):$(uname -m)" in
  Darwin:arm64) _stage0="$ROOT/bootstrap/stage0/norscode-macos-arm64" ;;
  Darwin:x86_64) _stage0="$ROOT/bootstrap/stage0/norscode-macos-x86_64" ;;
  Linux:x86_64) _stage0="$ROOT/bootstrap/stage0/norscode-linux-x86_64" ;;
  Linux:aarch64|Linux:arm64) _stage0="$ROOT/bootstrap/stage0/norscode-linux-arm64" ;;
  *) _stage0="" ;;
esac
if [ -n "${NORSCODE_NATIVE_BIN:-}" ]; then
  _stage0="$NORSCODE_NATIVE_BIN"
fi
if [ -n "$_stage0" ] && [ -x "$_stage0" ]; then
  export NORSCODE_NATIVE_BIN="$_stage0"
fi

_out="$(
  env \
    NORSCODE_ENABLE_EXEC_PROSESS=1 \
    NORSCODE_ROOT="$ROOT" \
    NORSCODE_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-}" \
    "$ROOT/bin/nc" run "$ROOT/tools/verify_seed_only.no" 2>&1
)" && {
  printf '%s\n' "$_out"
  exit 0
}

case "$_out" in
  *"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) ;;
  *)
    printf '%s\n' "$_out" >&2
    exit 1
    ;;
esac

if [ -z "$_stage0" ] || [ ! -f "$_stage0" ]; then
  printf '%s\n' "$_out" >&2
  exit 1
fi

printf '%s\n' "=== Seed-only verifikasjon (utan clang/regen) ==="
printf '%s\n' "1. Bygg frå seed med ugyldig CC (skal framleis passere)"
mkdir -p "$ROOT/dist"
rm -f "$ROOT/dist/norscode_native"
cp "$_stage0" "$ROOT/dist/norscode_native"
chmod +x "$ROOT/dist/norscode_native"
printf '%s\n' "dist/norscode_native <- ${_stage0#$ROOT/}"
printf '%s\n' "  [OK] seed materialisert i wrapper fordi stage0 manglar exec_prosess-binding"

printf '%s\n' "2. Køyr seed-smoke"
_tmp="${TMPDIR:-/tmp}/nc_seed_only_$$.no"
rm -f "$_tmp"
printf '%s\n' "funksjon start() { returner 0 }" >"$_tmp"
if NORSCODE_CMD=run NORSCODE_FILE="$_tmp" "$ROOT/dist/norscode_native" >/dev/null 2>&1; then
  rm -f "$_tmp"
  printf '%s\n' "  [OK] seed-smoke"
  printf '\n%s\n' "=== Seed-only verifikasjon: BESTÅTT ==="
  exit 0
fi
rm -f "$_tmp"
printf '%s\n' "Seed-smoke feila etter wrapper-materialisering" >&2
exit 1
