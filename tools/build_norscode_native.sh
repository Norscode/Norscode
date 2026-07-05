#!/usr/bin/env sh
# Tynn wrapper: native-runtime-materialisering ligg i tools/build_norscode_native.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ -z "${NORSCODE_NATIVE_BIN:-}" ] && [ ! -x "$ROOT/dist/norscode_native" ]; then
  case "$(uname -s):$(uname -m)" in
    Darwin:arm64) _stage0="$ROOT/bootstrap/stage0/norscode-macos-arm64" ;;
    Darwin:x86_64) _stage0="$ROOT/bootstrap/stage0/norscode-macos-x86_64" ;;
    Linux:x86_64) _stage0="$ROOT/bootstrap/stage0/norscode-linux-x86_64" ;;
    Linux:aarch64|Linux:arm64) _stage0="$ROOT/bootstrap/stage0/norscode-linux-arm64" ;;
    *) _stage0="" ;;
  esac
  if [ -n "$_stage0" ] && [ -x "$_stage0" ]; then
    export NORSCODE_NATIVE_BIN="$_stage0"
  fi
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/build_norscode_native.no"
