#!/usr/bin/env sh
# tools/docker-build-linux.sh
# Norscode-first wrapper: Linux bootstrap-pakking ligg i tools/docker-build-linux.no.
# Shell-delen under mappar output-argument til rot/miljø og startar Norscode-eigarfil.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ "${1:-}" = "--output" ]; then
    OUTPUT="${2:-$ROOT/dist/norscode-linux-x86_64}"
else
    OUTPUT="${1:-$ROOT/dist/norscode-linux-x86_64}"
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_DOCKER_BUILD_LINUX_OUTPUT="$OUTPUT"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/docker-build-linux.no"
