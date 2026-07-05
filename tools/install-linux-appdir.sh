#!/usr/bin/env sh
# Tynn wrapper: Linux AppDir-installasjon ligg i tools/install-linux-appdir.no.
set -eu

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/install-linux-appdir.sh <Norscode-linux-*-AppDir.tar.gz> [--prefix DIR]

Installerer eller oppgraderer Linux AppDir-artefakt lokalt.
Standard prefix: ~/.local/opt/norscode-app
EOF
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
ARCHIVE_PATH="$1"
shift
PREFIX="$HOME/.local/opt/norscode-app"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      PREFIX="$2"
      shift 2
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_LINUX_APPDIR_ARCHIVE="$ARCHIVE_PATH"
export NORSCODE_LINUX_APPDIR_PREFIX="$PREFIX"

exec "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/install-linux-appdir.no"
