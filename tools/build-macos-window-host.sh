#!/usr/bin/env sh
# Wrapper: Norscode eig byggereglane; shell gjer macOS-plattformutføring når native runtime manglar exec_prosess.
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
SOURCE_FILE="$ROOT_DIR/platform/macos/window-host/Main.swift"
DEFAULT_APP_TEMPLATE="$ROOT_DIR/platform/macos/window-host/app.no"
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
export NORSCODE_HOST_OS="$(uname -s)"
export NORSCODE_SWIFTC="$(command -v swiftc || true)"
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

_out="$(mktemp "${TMPDIR:-/tmp}/norscode_macos_window_host.XXXXXX")"
_rc=0
"$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/build-macos-window-host.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi

if ! grep -Eq 'Ukjent funksjon: builtin(\.builtin)?\.exec_prosess|Feil: manglar app-mal:|Feil: manglar Swift-kjelde:|Feil: manglar nc-køyrbar fil:' "$_out"; then
  cat "$_out" >&2
  rm -f "$_out"
  exit "$_rc"
fi
rm -f "$_out"

[ "$(uname -s)" = "Darwin" ] || {
  printf 'Feil: build-macos-window-host krev macOS.\n' >&2
  exit 1
}
[ -n "$NORSCODE_SWIFTC" ] || {
  printf 'Feil: manglar swiftc. Installer Xcode Command Line Tools.\n' >&2
  exit 1
}
[ -f "$SOURCE_FILE" ] || {
  printf 'Feil: manglar Swift-kjelde: %s\n' "$SOURCE_FILE" >&2
  exit 1
}
[ -f "$DEFAULT_APP_TEMPLATE" ] || {
  printf 'Feil: manglar app-mal: %s\n' "$DEFAULT_APP_TEMPLATE" >&2
  exit 1
}
[ -x "$DEFAULT_EXECUTABLE" ] || {
  printf 'Feil: manglar nc-køyrbar fil: %s\n' "$DEFAULT_EXECUTABLE" >&2
  exit 1
}

APP_DIR="$OUT_DIR/$APP_NAME.app"
CONTENTS_DIR="$APP_DIR/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"
MODULE_CACHE_DIR="$OUT_DIR/.swift-module-cache"
EXECUTABLE_PATH="$MACOS_DIR/$APP_NAME"

rm -rf "$APP_DIR"
mkdir -p "$MACOS_DIR" "$RESOURCES_DIR" "$MODULE_CACHE_DIR"
"$NORSCODE_SWIFTC" -O -module-cache-path "$MODULE_CACHE_DIR" -framework AppKit -framework WebKit "$SOURCE_FILE" -o "$EXECUTABLE_PATH"

if [ -f "$ICON_PATH" ]; then
  cp "$ICON_PATH" "$RESOURCES_DIR/$APP_NAME.icns"
fi

cat >"$RESOURCES_DIR/window-host.env" <<EOF
REPO_ROOT=$ROOT_DIR
APP_TEMPLATE=$DEFAULT_APP_TEMPLATE
NC_EXECUTABLE=$DEFAULT_EXECUTABLE
INITIAL_PATH=$INITIAL_PATH
INITIAL_QUERY=$INITIAL_QUERY
STATE_DIR=$OUT_DIR/window-state
EOF

cat >"$CONTENTS_DIR/Info.plist" <<EOF
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
