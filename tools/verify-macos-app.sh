#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/verify-macos-app.sh [APP_PATH]

Verifiserer structure, plist og codesign-status for ein macOS-appbundle.
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP_PATH="$ROOT_DIR/build/macos-app/Norscode.app"

if [ "$#" -gt 1 ]; then
  usage
  exit 1
fi

if [ "$#" -eq 1 ]; then
  APP_PATH="$1"
fi

case "$APP_PATH" in
  /*) ;;
  *) APP_PATH="$ROOT_DIR/$APP_PATH" ;;
esac

if [ "$(uname -s)" != "Darwin" ]; then
  printf 'Feil: verify-macos-app krev macOS.\n' >&2
  exit 1
fi

for cmd in plutil codesign spctl; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    printf 'Feil: manglar macOS-verktøyet %s\n' "$cmd" >&2
    exit 1
  fi
done

if [ ! -d "$APP_PATH" ]; then
  printf 'Feil: manglar app-bundle: %s\n' "$APP_PATH" >&2
  exit 1
fi

INFO_PLIST="$APP_PATH/Contents/Info.plist"
LAUNCHER="$APP_PATH/Contents/MacOS/$(basename "$APP_PATH" .app)"

if [ ! -f "$INFO_PLIST" ]; then
  printf 'Feil: manglar Info.plist\n' >&2
  exit 1
fi

if [ ! -x "$LAUNCHER" ]; then
  printf 'Feil: manglar launcher: %s\n' "$LAUNCHER" >&2
  exit 1
fi

plutil -lint "$INFO_PLIST" >/dev/null

printf 'Info.plist: OK\n'
printf 'Launcher: OK\n'

printf '\nCodesign verify:\n'
codesign --verify --deep --strict "$APP_PATH"

printf '\nCodesign details:\n'
codesign -dvv "$APP_PATH" 2>&1

printf '\nGatekeeper assess:\n'
if ! spctl --assess --type execute --verbose=4 "$APP_PATH"; then
  printf 'Merk: Gatekeeper-avvik er forventa for ad-hoc-signert eller ikkje-notarisert app.\n'
fi

printf '\nmacOS app-verifikasjon: OK\n'
