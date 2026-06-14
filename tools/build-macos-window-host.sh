#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/build-macos-window-host.sh [--out DIR] [--name NAMN] [--bundle-id ID] [--version VER]

Byggjer ein minimal macOS GUI-host for Norscode med ekte vindauge.
Standard output: build/macos-window-host/NorscodeWindowHost.app
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
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

if [ "$(uname -s)" != "Darwin" ]; then
  printf 'Feil: build-macos-window-host krev macOS.\n' >&2
  exit 1
fi

if ! command -v swiftc >/dev/null 2>&1; then
  printf 'Feil: manglar swiftc. Installer Xcode Command Line Tools.\n' >&2
  exit 1
fi

case "$DEFAULT_APP_TEMPLATE" in
  /*) ;;
  *) DEFAULT_APP_TEMPLATE="$ROOT_DIR/$DEFAULT_APP_TEMPLATE" ;;
esac

case "$DEFAULT_EXECUTABLE" in
  /*) ;;
  *) DEFAULT_EXECUTABLE="$ROOT_DIR/$DEFAULT_EXECUTABLE" ;;
esac

if [ ! -f "$DEFAULT_APP_TEMPLATE" ]; then
  printf 'Feil: manglar app-mal: %s\n' "$DEFAULT_APP_TEMPLATE" >&2
  exit 1
fi

if [ ! -x "$DEFAULT_EXECUTABLE" ]; then
  printf 'Feil: manglar nc-køyrbar fil: %s\n' "$DEFAULT_EXECUTABLE" >&2
  exit 1
fi

APP_DIR="$OUT_DIR/$APP_NAME.app"
CONTENTS_DIR="$APP_DIR/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"
EXECUTABLE_PATH="$MACOS_DIR/$APP_NAME"
MODULE_CACHE_DIR="$OUT_DIR/.swift-module-cache"

rm -rf "$APP_DIR"
mkdir -p "$MACOS_DIR" "$RESOURCES_DIR" "$MODULE_CACHE_DIR"

swiftc \
  -O \
  -module-cache-path "$MODULE_CACHE_DIR" \
  -framework AppKit \
  -framework WebKit \
  "$SOURCE_FILE" \
  -o "$EXECUTABLE_PATH"

if [ -f "$ICON_PATH" ]; then
  cp "$ICON_PATH" "$RESOURCES_DIR/$APP_NAME.icns"
fi

cat > "$RESOURCES_DIR/window-host.env" <<EOF
REPO_ROOT=$ROOT_DIR
APP_TEMPLATE=$DEFAULT_APP_TEMPLATE
NC_EXECUTABLE=$DEFAULT_EXECUTABLE
INITIAL_PATH=$INITIAL_PATH
INITIAL_QUERY=$INITIAL_QUERY
STATE_DIR=$OUT_DIR/window-state
EOF

cat > "$CONTENTS_DIR/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>nb</string>
  <key>CFBundleDisplayName</key>
  <string>$APP_NAME</string>
  <key>CFBundleExecutable</key>
  <string>$APP_NAME</string>
  <key>CFBundleIdentifier</key>
  <string>$BUNDLE_ID</string>
  <key>CFBundleIconFile</key>
  <string>$APP_NAME</string>
  <key>CFBundleName</key>
  <string>$APP_NAME</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>$APP_VERSION</string>
  <key>CFBundleVersion</key>
  <string>$APP_VERSION</string>
  <key>LSApplicationCategoryType</key>
  <string>public.app-category.developer-tools</string>
  <key>LSMinimumSystemVersion</key>
  <string>11.0</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
EOF

if command -v plutil >/dev/null 2>&1; then
  plutil -lint "$CONTENTS_DIR/Info.plist" >/dev/null
fi

printf 'Bygde macOS window host: %s\n' "$APP_DIR"
