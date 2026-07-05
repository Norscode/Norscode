#!/usr/bin/env sh
# Tynn wrapper: v3601 Docker-bygg ligg i .no-fila.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
exec "$ROOT/bin/nc" run "$ROOT/tools/build_linux_x86_64_stage0_docker_v3601.no"
