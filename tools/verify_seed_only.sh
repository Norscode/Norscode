#!/usr/bin/env sh
# tools/verify_seed_only.sh — tynn wrapper for Norscode-native seed-sjekk
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
  printf '=== Seed-only verifikasjon (utan clang/regen) ===\n'
  printf '1. Bygg frå seed med ugyldig CC (skal framleis passere)\n'
  CC=__clang_not_allowed__ REGEN=0 bash "$ROOT/tools/build_norscode_native.sh"
  printf '2. Køyr seed-smoke\n'
  _tmp="$(mktemp "${TMPDIR:-/tmp}/nc_seed_only_XXXXXX.no")"
  printf 'funksjon start() { returner 0 }\n' > "$_tmp"
  NORSCODE_CMD=run NORSCODE_FILE="$_tmp" "$ROOT/dist/norscode_native" >/dev/null
  rm -f "$_tmp"
  printf '  [OK] seed-smoke\n\n'
  printf '=== Seed-only verifikasjon: BESTÅTT ===\n'
  exit 0
fi
exec env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_ROOT="$ROOT" \
  NORSCODE_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-}" \
  "$ROOT/bin/nc" run "$ROOT/tools/verify_seed_only.no"
