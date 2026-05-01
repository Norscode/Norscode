#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_NAME="Norscode IDE.app"
SOURCE="$ROOT_DIR/dist_desktop_ide/$APP_NAME"
TARGET_BASE="/Applications"
if [[ ! -w "$TARGET_BASE" ]]; then
  TARGET_BASE="$HOME/Applications"
fi
TARGET="$TARGET_BASE/$APP_NAME"

if [[ ! -d "$SOURCE" ]]; then
  echo "Fant ikke pakke: $SOURCE"
  echo "Kjør først: bash scripts/package-desktop-ide.sh app"
  exit 1
fi

mkdir -p "$TARGET_BASE"
if [[ -d "$TARGET" ]]; then
  rm -rf "$TARGET"
fi
cp -R "$SOURCE" "$TARGET"
echo "Installert: $TARGET"
if [[ "$TARGET_BASE" == "$HOME/Applications" ]]; then
  echo "Appen ble plassert i $HOME/Applications fordi /Applications ikke var skrivbar."
fi
open -a "$TARGET"
