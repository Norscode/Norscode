#!/usr/bin/env sh
# Norscode-first wrapper: stage0-seed-henting ligg i tools/fetch_stage0_seed.no.
# Shell-delen under set berre rot/plattform/seedmetadata og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

first_word() {
  _text="$1"
  set -- $_text
  printf '%s' "${1:-}"
}

file_bytes() {
  _file="$1"
  first_word "$(wc -c "$_file")"
}

if [ "${1:-}" != "" ]; then
  export NORSCODE_STAGE0_PLATFORM="$1"
fi
if [ "${NORSCODE_STAGE0_PLATFORM:-}" = "" ]; then
  _os="$(uname -s)"
  _arch="$(uname -m)"
  case "$_os:$_arch" in
    Darwin:arm64) NORSCODE_STAGE0_PLATFORM="macos-arm64" ;;
    Darwin:x86_64) NORSCODE_STAGE0_PLATFORM="macos-x86_64" ;;
    Linux:x86_64|Linux:amd64) NORSCODE_STAGE0_PLATFORM="linux-x86_64" ;;
    Linux:aarch64|Linux:arm64) NORSCODE_STAGE0_PLATFORM="linux-arm64" ;;
  esac
  export NORSCODE_STAGE0_PLATFORM
fi
if [ "${NORSCODE_STAGE0_PLATFORM:-}" != "" ]; then
  _seed="$ROOT/bootstrap/stage0/norscode-$NORSCODE_STAGE0_PLATFORM"
  if [ -f "$_seed" ]; then
    NORSCODE_STAGE0_SEED_BYTES="$(file_bytes "$_seed")"
    export NORSCODE_STAGE0_SEED_BYTES
  fi
fi

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/fetch_stage0_seed.no"
