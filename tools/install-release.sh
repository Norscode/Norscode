#!/usr/bin/env sh
set -eu

usage() {
  printf 'Bruk: bash tools/install-release.sh <release.tar.gz> [--prefix DIR]\n' >&2
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

ARCHIVE_PATH="$1"
shift

PREFIX="${HOME}/.local/share/norscode"
while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      if [ "$#" -lt 2 ]; then
        usage
        exit 1
      fi
      PREFIX="$2"
      shift 2
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_INSTALL_ARCHIVE="$ARCHIVE_PATH" \
  NORSCODE_INSTALL_PREFIX="$PREFIX" \
  NORSCODE_ROOT="$ROOT_DIR" \
  "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/install_release.no"
