#!/usr/bin/env sh
# Tynn wrapper: Linux desktop-entry installasjon ligg i tools/install-linux-desktop-entry.no.
set -eu

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/install-linux-desktop-entry.sh [APPDIR] [--prefix DIR]

Installerer Linux desktop entry og ikon for Norscode frå ein bygd AppDir.
Standard prefix: ~/.local/share
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
APPDIR="$ROOT_DIR/build/linux-app/Norscode.AppDir"
PREFIX="$HOME/.local/share"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      PREFIX="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      if [ "$APPDIR" = "$ROOT_DIR/build/linux-app/Norscode.AppDir" ]; then
        APPDIR="$1"
        shift
      else
        usage
        exit 1
      fi
      ;;
  esac
done

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_LINUX_DESKTOP_APPDIR="$APPDIR"
export NORSCODE_LINUX_DESKTOP_PREFIX="$PREFIX"

exec "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/install-linux-desktop-entry.no"
