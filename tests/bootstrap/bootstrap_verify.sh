#!/usr/bin/env sh
# Tynn wrapper: bootstrap-verifisering ligg i tests/bootstrap/bootstrap_verify.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tests/bootstrap/bootstrap_verify.no"
