#!/usr/bin/env bash
# Tynn wrapper: kompatibilitetsgaten ligg i tools/build-bootstrap-binary.no.
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"

exec "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/build-bootstrap-binary.no"
