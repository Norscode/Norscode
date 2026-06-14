#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/install-macos-app.sh [APP_PATH] [--dest DIR] [--name NAMN]

Installerer eller oppdaterer ein lokal Norscode.app til ~/Applications som standard.
Dersom APP_PATH ikkje er gitt, blir build/macos-app/Norscode.app brukt.
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
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

if [ "$(uname -s)" != "Darwin" ]; then
  printf 'Feil: install-macos-app krev macOS.\n' >&2
  exit 1
fi

if [ ! -d "$APP_PATH" ]; then
  printf 'Feil: manglar app-bundle: %s\n' "$APP_PATH" >&2
  printf 'Bygg først med: bash tools/build-macos-app.sh\n' >&2
  exit 1
fi

if [ ! -f "$APP_PATH/Contents/Info.plist" ]; then
  printf 'Feil: ugyldig app-bundle, manglar Info.plist: %s\n' "$APP_PATH" >&2
  exit 1
fi

if ! command -v plutil >/dev/null 2>&1; then
  printf 'Feil: manglar macOS-verktøyet plutil\n' >&2
  exit 1
fi

plutil -lint "$APP_PATH/Contents/Info.plist" >/dev/null

TARGET_APP="$DEST_DIR/$APP_NAME"
VERSIONED_ROOT="$DEST_DIR/.Norscode"
VERSIONS_DIR="$VERSIONED_ROOT/versions"
CURRENT_LINK="$VERSIONED_ROOT/current"
APP_VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP_PATH/Contents/Info.plist" 2>/dev/null || printf '1.0')"
VERSIONED_APP="$VERSIONS_DIR/$APP_VERSION/$APP_NAME"

mkdir -p "$DEST_DIR" "$VERSIONS_DIR/$APP_VERSION"
rm -rf "$VERSIONED_APP"
cp -R "$APP_PATH" "$VERSIONED_APP"

ln -sfn "$VERSIONED_APP" "$CURRENT_LINK"
rm -rf "$TARGET_APP"
ln -sfn "$CURRENT_LINK" "$TARGET_APP"

if [ -x "$VERSIONED_APP/Contents/Resources/runtime/tools/install-macos-file-icons.sh" ]; then
  bash "$VERSIONED_APP/Contents/Resources/runtime/tools/install-macos-file-icons.sh" || true
fi

printf 'Installert macOS-app: %s\n' "$VERSIONED_APP"
printf 'Aktiv app-lenke: %s\n' "$TARGET_APP"
printf 'Start appen med: open %q\n' "$TARGET_APP"
