#!/usr/bin/env sh
set -eu

if [ "$#" -lt 1 ]; then
  printf 'Bruk: bash package-release.sh <versjon>\n' >&2
  exit 1
fi

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
exec env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_RELEASE_VERSION="$1" \
  NORSCODE_ROOT="$ROOT_DIR" \
  "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/package_release.no"
