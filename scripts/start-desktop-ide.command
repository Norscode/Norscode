#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_NAME="Norscode IDE.app"
APP_BUNDLE="$ROOT_DIR/dist_desktop_ide/$APP_NAME"
IDE_PY="$ROOT_DIR/desktop_ide/main.py"

if [[ -d "$APP_BUNDLE" ]]; then
  echo "Starter installert app: $APP_BUNDLE"
  open -a "$APP_BUNDLE"
  exit 0
fi

echo "Fant ikke pakket app ($APP_BUNDLE)."
echo "Starter direkte Python-app i stedet..."

if ! command -v python3 >/dev/null 2>&1; then
  echo "Fant ikke python3. Installer Python og prøv igjen."
  exit 1
fi

python3 "$IDE_PY"
