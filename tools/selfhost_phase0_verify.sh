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
exec env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT_DIR" NORSCODE_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-}" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/selfhost_phase0_verify.no"
