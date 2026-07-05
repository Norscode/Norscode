#!/usr/bin/env sh
# Tynn wrapper: macOS app-installasjon ligg i tools/install-macos-app.no.
set -eu

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/install-macos-app.sh [APP_PATH] [--dest DIR] [--name NAMN]

Installerer eller oppdaterer ein lokal Norscode.app til ~/Applications som standard.
Dersom APP_PATH ikkje er gitt, blir build/macos-app/Norscode.app brukt.
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
APP_PATH="$ROOT_DIR/build/macos-app/Norscode.app"
DEST_DIR="$HOME/Applications"
APP_NAME="Norscode.app"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --dest)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      DEST_DIR="$2"
      shift 2
      ;;
    --name)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_NAME="$2"
      case "$APP_NAME" in
        *.app) ;;
        *) APP_NAME="${APP_NAME}.app" ;;
      esac
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      if [ "$APP_PATH" = "$ROOT_DIR/build/macos-app/Norscode.app" ]; then
        APP_PATH="$1"
        shift
      else
        usage
        exit 1
      fi
      ;;
  esac
done

case "$APP_PATH" in
  /*) ;;
  *) APP_PATH="$ROOT_DIR/$APP_PATH" ;;
esac

case "$DEST_DIR" in
  /*) ;;
  *) DEST_DIR="$ROOT_DIR/$DEST_DIR" ;;
esac

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_MACOS_INSTALL_APP="$APP_PATH"
export NORSCODE_MACOS_INSTALL_DEST="$DEST_DIR"
export NORSCODE_MACOS_INSTALL_NAME="$APP_NAME"

exec "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/install-macos-app.no"
