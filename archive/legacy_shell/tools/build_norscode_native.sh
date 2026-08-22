#!/usr/bin/env sh
# Norscode-first wrapper: native-runtime-materialisering ligg i tools/build_norscode_native.no.
# Shell-delen under er avgrensa stage0-reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

first_word() {
  _text="$1"
  set -- $_text
  printf '%s' "${1:-}"
}

file_bytes() {
  _file="$1"
  first_word "$(wc -c "$_file")"
}

file_kb() {
  _bytes="$(file_bytes "$1")"
  printf '%s' "$(( _bytes / 1024 ))"
}

if [ -x "$ROOT/dist/norscode_native" ]; then
  _tmp="$(mktemp "${TMPDIR:-/tmp}/nc_dist_smoke_XXXXXX.no")"
  printf 'funksjon start() { returner 0 }\n' > "$_tmp"
  NORSCODE_CMD=run NORSCODE_FILE="$_tmp" "$ROOT/dist/norscode_native" >/dev/null
  rm -f "$_tmp"
  printf 'dist/norscode_native er allereie bygget (%s KB)\n' "$(file_kb "$ROOT/dist/norscode_native")"
  exit 0
fi

if [ -n "${NORSCODE_NATIVE_BIN:-}" ] && [ ! -x "$ROOT/dist/norscode_native" ] && [ -x "$NORSCODE_NATIVE_BIN" ]; then
  mkdir -p "$ROOT/dist"
  rm -f "$ROOT/dist/norscode_native"
  cp "$NORSCODE_NATIVE_BIN" "$ROOT/dist/norscode_native"
  chmod +x "$ROOT/dist/norscode_native"
  printf 'dist/norscode_native <- %s (%s bytes)\n' "${NORSCODE_NATIVE_BIN#$ROOT/}" "$(file_bytes "$ROOT/dist/norscode_native")"
  printf 'Klar: native runtime er materialisert utan C/Python.\n'
  exit 0
fi

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
    mkdir -p "$ROOT/dist"
    rm -f "$ROOT/dist/norscode_native"
    cp "$_stage0" "$ROOT/dist/norscode_native"
    chmod +x "$ROOT/dist/norscode_native"
    printf 'dist/norscode_native <- %s (%s bytes)\n' "${_stage0#$ROOT/}" "$(file_bytes "$ROOT/dist/norscode_native")"
    printf 'Klar: native runtime er materialisert utan C/Python.\n'
    exit 0
  fi
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/build_norscode_native.no"
