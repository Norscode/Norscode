#!/usr/bin/env bash
# Tynn wrapper: manifestlogikken ligg i tools/update_stage0_manifest.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

_manifest_input=""
for _seed in "$ROOT"/bootstrap/stage0/norscode-*; do
  [ -f "$_seed" ] || continue
  if command -v sha256sum >/dev/null 2>&1; then
    set -- $(sha256sum "$_seed")
  else
    set -- $(shasum -a 256 "$_seed")
  fi
  _manifest_input="${_manifest_input}${1}  ${_seed}
"
done
export NORSCODE_STAGE0_MANIFEST_INPUT="$_manifest_input"
export NORSCODE_STAGE0_MANIFEST_ALLOW_EMPTY=1

exec "$ROOT/bin/nc" run "$ROOT/tools/update_stage0_manifest.no"
