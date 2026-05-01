#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_DIR="$ROOT_DIR/desktop_ide"
DIST_DIR="$ROOT_DIR/dist_desktop_ide"
WEB_APP_DIR="$ROOT_DIR/../../norscode-website"
mkdir -p "$DIST_DIR"

if ! command -v python3 >/dev/null 2>&1; then
  echo "Python3 er påkrevd."
  exit 1
fi

if ! python3 -m pip show pyinstaller >/dev/null 2>&1; then
  echo "Installerer pyinstaller..."
  python3 -m pip install --user pyinstaller >/dev/null
fi

OS_NAME="$(uname -s)"
MODE="${1:-all}"

BUILD_NAME="Norscode IDE"
cd "$ROOT_DIR"

python3 -m PyInstaller \
  --name "$BUILD_NAME" \
  --windowed \
  --noconfirm \
  --distpath "$DIST_DIR" \
  --workpath "$DIST_DIR/.build" \
  --add-data "$WEB_APP_DIR:norscode-website" \
  "$APP_DIR/main.py"

if [[ "$MODE" == "app" || "$MODE" == "all" ]]; then
  if [[ "$OS_NAME" == "Darwin" ]]; then
    APP_BUNDLE="$DIST_DIR/${BUILD_NAME}.app"
    if [[ -d "$APP_BUNDLE" ]]; then
      hdiutil create -volname "Norscode IDE" -srcfolder "$APP_BUNDLE" -ov -format UDZO "$DIST_DIR/Norscode-IDE.dmg"
      echo "Opprettet: $DIST_DIR/Norscode-IDE.dmg"
      echo "Installeringsklar app: $APP_BUNDLE"
    else
      echo "Fant ikke macOS app-bundle. Sjekk pyinstaller-støtte på macOS." >&2
      exit 1
    fi
  else
    echo "APP-modus støttes med .app-bundle kun på macOS."
    echo "For Windows, bruk mode='exe' på en Windows-maskin."
  fi
fi

if [[ "$MODE" == "exe" || "$MODE" == "all" ]]; then
  echo "Hint: For Windows-exe må du bygge på Windows med pyinstaller."
  echo "Bruk scripts/package-desktop-ide.ps1 der du kjører builden på Windows."
fi

echo "Pakkingen er ferdig. Filer ligger i: $DIST_DIR"
echo "Manuell kjøring av appen (macOS): $DIST_DIR/${BUILD_NAME}.app/Contents/MacOS/${BUILD_NAME}"
