#!/usr/bin/env sh
# Tynn wrapper: macOS window-host-bygg ligg i tools/build-macos-window-host.no.
set -eu

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/build-macos-window-host.sh [--out DIR] [--name NAMN] [--bundle-id ID] [--version VER]

Byggjer ein minimal macOS GUI-host for Norscode med ekte vindauge.
Standard output: build/macos-window-host/NorscodeWindowHost.app
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT_DIR="$ROOT_DIR/build/macos-window-host"
APP_NAME="NorscodeWindowHost"
BUNDLE_ID="dev.norscode.window-host"
APP_VERSION="1.0"
ICON_PATH="$ROOT_DIR/frontend/assets/icons/norscode.icns"
SOURCE_FILE="$ROOT_DIR/tools/macos-window-host/Main.swift"
DEFAULT_APP_TEMPLATE="$ROOT_DIR/tools/macos-window-host/app.no"
DEFAULT_EXECUTABLE="$ROOT_DIR/bin/nc"
INITIAL_PATH="/"
INITIAL_QUERY=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --out)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      OUT_DIR="$2"
      shift 2
      ;;
    --name)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_NAME="$2"
      shift 2
      ;;
    --bundle-id)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      BUNDLE_ID="$2"
      shift 2
      ;;
    --version)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_VERSION="$2"
      shift 2
      ;;
    --app-file)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      DEFAULT_APP_TEMPLATE="$2"
      shift 2
      ;;
    --nc)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      DEFAULT_EXECUTABLE="$2"
      shift 2
      ;;
    --path)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      INITIAL_PATH="$2"
      shift 2
      ;;
    --query)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      INITIAL_QUERY="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_MACOS_WINDOW_OUT="$OUT_DIR"
export NORSCODE_MACOS_WINDOW_NAME="$APP_NAME"
export NORSCODE_MACOS_WINDOW_BUNDLE_ID="$BUNDLE_ID"
export NORSCODE_MACOS_WINDOW_VERSION="$APP_VERSION"
export NORSCODE_MACOS_WINDOW_ICON="$ICON_PATH"
export NORSCODE_MACOS_WINDOW_SOURCE="$SOURCE_FILE"
export NORSCODE_MACOS_WINDOW_TEMPLATE="$DEFAULT_APP_TEMPLATE"
export NORSCODE_MACOS_WINDOW_NC="$DEFAULT_EXECUTABLE"
export NORSCODE_MACOS_WINDOW_PATH="$INITIAL_PATH"
export NORSCODE_MACOS_WINDOW_QUERY="$INITIAL_QUERY"

exec "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/build-macos-window-host.no"
