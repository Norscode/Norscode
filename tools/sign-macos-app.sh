#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/sign-macos-app.sh [APP_PATH] [--identity NAME] [--entitlements FILE] [--verify]

Signerer ein lokal macOS-appbundle.

- utan --identity bruker skriptet ad-hoc-signering ("-")
- med --identity kan du bruke Developer ID Application eller anna lokal codesign-identitet
- med --verify køyrer skriptet verifikasjon etter signering
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP_PATH="$ROOT_DIR/build/macos-app/Norscode.app"
IDENTITY="-"
VERIFY=0
ENTITLEMENTS="$ROOT_DIR/tools/macos-app.entitlements"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --identity)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      IDENTITY="$2"
      shift 2
      ;;
    --entitlements)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      ENTITLEMENTS="$2"
      shift 2
      ;;
    --verify)
      VERIFY=1
      shift
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

case "$ENTITLEMENTS" in
  /*) ;;
  *) ENTITLEMENTS="$ROOT_DIR/$ENTITLEMENTS" ;;
esac

if [ "$(uname -s)" != "Darwin" ]; then
  printf 'Feil: sign-macos-app krev macOS.\n' >&2
  exit 1
fi

for cmd in codesign plutil; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    printf 'Feil: manglar macOS-verktøyet %s\n' "$cmd" >&2
    exit 1
  fi
done

if [ ! -d "$APP_PATH" ]; then
  printf 'Feil: manglar app-bundle: %s\n' "$APP_PATH" >&2
  exit 1
fi

if [ ! -f "$APP_PATH/Contents/Info.plist" ]; then
  printf 'Feil: ugyldig app-bundle, manglar Info.plist: %s\n' "$APP_PATH" >&2
  exit 1
fi

plutil -lint "$APP_PATH/Contents/Info.plist" >/dev/null

if [ ! -f "$ENTITLEMENTS" ]; then
  printf 'Feil: manglar entitlements-fil: %s\n' "$ENTITLEMENTS" >&2
  exit 1
fi

printf 'Signerer macOS-app: %s\n' "$APP_PATH"
printf 'Identity: %s\n' "$IDENTITY"

codesign \
  --force \
  --deep \
  --sign "$IDENTITY" \
  --timestamp=none \
  --entitlements "$ENTITLEMENTS" \
  "$APP_PATH"

printf 'Signering ferdig.\n'

if [ "$VERIFY" -eq 1 ]; then
  bash "$ROOT_DIR/tools/verify-macos-app.sh" "$APP_PATH"
fi
