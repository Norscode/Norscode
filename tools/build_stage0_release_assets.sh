#!/usr/bin/env bash
# Tynn wrapper: release-asset-bygg ligg i tools/build_stage0_release_assets.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

exec "$ROOT/bin/nc" run "$ROOT/tools/build_stage0_release_assets.no"
