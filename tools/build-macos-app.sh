#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/build-macos-app.sh [--config FIL] [--mode gui|terminal] [--name NAMN] [--out DIR] [--bundle-id ID] [--version VER]

Lagar ein minimal, usignert macOS .app-bundle rundt den lokale Norscode-runtime-en.
Standard output: build/macos-app/Norscode.app
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP_NAME="Norscode"
OUT_DIR="$ROOT_DIR/build/macos-app"
BUNDLE_ID="dev.norscode.app"
APP_DIR=""
APP_VERSION="1.0"
CONFIG_FILE=""
ICON_PATH="$ROOT_DIR/frontend/assets/icons/norscode.icns"
DOC_TYPE_SOURCE_NAME="Norscode Source"
DOC_TYPE_UTILITY_NAME="Norscode Utility Source"
DOC_TYPE_BYTECODE_NAME="Norscode Bytecode JSON"
WELCOME_TITLE="Norscode macOS app-bundle"
WELCOME_MESSAGE="Velkomen til Norscode på macOS."
APP_MODE="gui"

read_repo_version() {
  local toml="$ROOT_DIR/norcode.toml"
  if [ -f "$toml" ]; then
    grep '^version' "$toml" | head -1 | sed 's/.*= *"\(.*\)"/\1/'
  else
    printf '1.0'
  fi
}

APP_VERSION="$(read_repo_version)"

load_config() {
  local config_path="$1"
  if [ ! -f "$config_path" ]; then
    printf 'Feil: manglar config-fil: %s\n' "$config_path" >&2
    exit 1
  fi
  # shellcheck disable=SC1090
  . "$config_path"
}

parse_config_flag() {
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --config)
        [ "$#" -ge 2 ] || { usage; exit 1; }
        CONFIG_FILE="$2"
        return 0
        ;;
    esac
    shift
  done
}

parse_config_flag "$@"

if [ -n "$CONFIG_FILE" ]; then
  case "$CONFIG_FILE" in
    /*) ;;
    *) CONFIG_FILE="$ROOT_DIR/$CONFIG_FILE" ;;
  esac
  load_config "$CONFIG_FILE"
fi

while [ "$#" -gt 0 ]; do
  case "$1" in
    --config)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      shift 2
      ;;
    --name)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_NAME="$2"
      shift 2
      ;;
    --mode)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_MODE="$2"
      shift 2
      ;;
    --out)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      OUT_DIR="$2"
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
  printf 'Feil: build-macos-app krev macOS.\n' >&2
  exit 1
fi

if [ ! -x "$ROOT_DIR/bin/nc" ]; then
  printf 'Feil: manglar %s/bin/nc\n' "$ROOT_DIR" >&2
  exit 1
fi

if [ ! -x "$ROOT_DIR/dist/norscode_native" ]; then
  printf 'Feil: manglar dist/norscode_native. Køyr: bash tools/build_norscode_native.sh\n' >&2
  exit 1
fi

if ! command -v plutil >/dev/null 2>&1; then
  printf 'Feil: manglar macOS-verktøyet plutil\n' >&2
  exit 1
fi

APP_DIR="$OUT_DIR/$APP_NAME.app"
CONTENTS_DIR="$APP_DIR/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"
RUNTIME_DIR="$RESOURCES_DIR/runtime"
WINDOW_HOST_SOURCE="$ROOT_DIR/tools/macos-window-host/Main.swift"
WINDOW_HOST_TEMPLATE="$ROOT_DIR/tools/macos-window-host/app.no"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

rm -rf "$APP_DIR"
mkdir -p "$MACOS_DIR" "$RESOURCES_DIR" "$RUNTIME_DIR"

for entry in \
  bin \
  bootstrap \
  dist \
  selfhost \
  std \
  norcode.toml \
  README.md \
  LICENSE
do
  if [ -e "$ROOT_DIR/$entry" ]; then
    cp -R "$ROOT_DIR/$entry" "$RUNTIME_DIR/"
  fi
done

case "$ICON_PATH" in
  /*) ;;
  *) ICON_PATH="$ROOT_DIR/$ICON_PATH" ;;
esac

if [ -f "$ICON_PATH" ]; then
  cp "$ICON_PATH" "$RESOURCES_DIR/$APP_NAME.icns"
fi

build_terminal_launcher() {
cat > "$MACOS_DIR/$APP_NAME" <<EOF
#!/usr/bin/env sh
set -eu
APP_DIR="\$(CDPATH= cd -- "\$(dirname -- "\$0")/.." && pwd)"
RUNTIME_DIR="\$APP_DIR/Resources/runtime"
COMMAND_FILE="\$APP_DIR/Resources/${APP_NAME}.command"

if [ "\$#" -gt 0 ]; then
  exec /usr/bin/open -a Terminal "\$COMMAND_FILE" --args "\$@"
fi

exec /usr/bin/open -a Terminal "\$COMMAND_FILE"
EOF
chmod +x "$MACOS_DIR/$APP_NAME"

cat > "$RESOURCES_DIR/${APP_NAME}.command" <<EOF
#!/usr/bin/env sh
set -eu
SCRIPT_DIR="\$(CDPATH= cd -- "\$(dirname -- "\$0")" && pwd)"
RUNTIME_DIR="\$SCRIPT_DIR/runtime"
clear
printf '%s\n' '$WELCOME_TITLE'
printf 'Runtime: %s\n\n' "\$RUNTIME_DIR"
cd "\$RUNTIME_DIR"
if [ "\$#" -gt 0 ]; then
  printf 'Opna frå Finder med:\n'
  for arg in "\$@"; do
    printf '  %s\n' "\$arg"
  done
  printf '\n'
  first_arg="\$1"
  case "\$first_arg" in
    *.no)
      printf 'Køyrer ./bin/nc run %s\n\n' "\$first_arg"
      exec ./bin/nc run "\$first_arg"
      ;;
    *.ncb.json)
      printf 'Viser bytekodefil: %s\n' "\$first_arg"
      printf 'Bruk ./bin/nc eller eigne verktøy frå denne Terminalen.\n\n'
      ;;
    *)
      printf 'Ingen automatisk handtering for denne filtypen enno.\n\n'
      ;;
  esac
fi

printf '%s\n\n' '$WELCOME_MESSAGE'
printf 'Vanlege kommandoar:\n'
printf '  ./bin/nc --help\n'
printf '  ./bin/nc test\n'
printf '  ./bin/nc run app.no\n'
printf '  ./bin/nc check app.no\n'
printf '  ./bin/nc build app.no out.ncb.json\n\n'
printf 'Denne appen er foreløpig ei utviklarflate rundt den bundla CLI-en.\n'
printf 'Du er no i runtime-mappa: %s\n\n' "\$PWD"
exec "\${SHELL:-/bin/zsh}" -lc 'printf "Norscode-bundle klar i %s\n" "\$PWD"; exec "\${SHELL:-/bin/zsh}"'
EOF
chmod +x "$RESOURCES_DIR/${APP_NAME}.command"
}

build_gui_launcher() {
  local module_cache_dir="$OUT_DIR/.swift-module-cache"
  if ! command -v swiftc >/dev/null 2>&1; then
    printf 'Feil: manglar swiftc. Installer Xcode Command Line Tools.\n' >&2
    exit 1
  fi
  if [ ! -f "$WINDOW_HOST_SOURCE" ] || [ ! -f "$WINDOW_HOST_TEMPLATE" ]; then
    printf 'Feil: manglar macOS window host-kjelder under tools/macos-window-host/\n' >&2
    exit 1
  fi

  mkdir -p "$module_cache_dir"
  swiftc \
    -O \
    -module-cache-path "$module_cache_dir" \
    -framework AppKit \
    -framework WebKit \
    "$WINDOW_HOST_SOURCE" \
    -o "$MACOS_DIR/$APP_NAME"

  cp "$WINDOW_HOST_TEMPLATE" "$RESOURCES_DIR/window-app-template.no"

  cat > "$RESOURCES_DIR/window-host.env" <<EOF
RUNTIME_ROOT=$RUNTIME_DIR
APP_TEMPLATE=$RESOURCES_DIR/window-app-template.no
NC_EXECUTABLE=$RUNTIME_DIR/bin/nc
INITIAL_PATH=/
INITIAL_QUERY=
STATE_DIR=$TMP_DIR/window-state
EOF
}

case "$APP_MODE" in
  gui)
    build_gui_launcher
    ;;
  terminal)
    build_terminal_launcher
    ;;
  *)
    printf 'Feil: ugyldig mode %s (bruk gui eller terminal)\n' "$APP_MODE" >&2
    exit 1
    ;;
esac

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
  <key>CFBundleDocumentTypes</key>
  <array>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>no</string></array>
      <key>CFBundleTypeIconFile</key>
      <string>$APP_NAME</string>
      <key>CFBundleTypeName</key>
      <string>$DOC_TYPE_SOURCE_NAME</string>
      <key>CFBundleTypeRole</key>
      <string>Editor</string>
      <key>LSHandlerRank</key>
      <string>Owner</string>
    </dict>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>nc</string></array>
      <key>CFBundleTypeIconFile</key>
      <string>$APP_NAME</string>
      <key>CFBundleTypeName</key>
      <string>$DOC_TYPE_UTILITY_NAME</string>
      <key>CFBundleTypeRole</key>
      <string>Editor</string>
      <key>LSHandlerRank</key>
      <string>Owner</string>
    </dict>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>ncb.json</string></array>
      <key>CFBundleTypeIconFile</key>
      <string>$APP_NAME</string>
      <key>CFBundleTypeName</key>
      <string>$DOC_TYPE_BYTECODE_NAME</string>
      <key>CFBundleTypeRole</key>
      <string>Viewer</string>
      <key>LSHandlerRank</key>
      <string>Alternate</string>
    </dict>
  </array>
</dict>
</plist>
EOF

plutil -lint "$CONTENTS_DIR/Info.plist" >/dev/null

printf 'Bygde macOS app-bundle (%s): %s\n' "$APP_MODE" "$APP_DIR"
printf 'Start lokalt med: open %q\n' "$APP_DIR"
printf 'Opne fil med appen: open -a %q path/to/fil.no\n' "$APP_DIR"
printf 'App-versjon: %s\n' "$APP_VERSION"
