#!/usr/bin/env sh
# Tynn wrapper: artefaktinnsamling ligg i tools/bootstrap_artifact_pipeline.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/bootstrap_artifact_pipeline.no"
