#!/usr/bin/env sh
# Norscode-first wrapper: macOS app-signering ligg i tools/sign-macos-app.no.
# Shell-delen under set berre rot/prosessmiljø/argument og startar Norscode-eigarfil.
set -eu

usage() {
  cat >&2 <<'EOF'
bruk: nc sign-macos-app [APP_PATH] [--identity NAME] [--entitlements FILE] [--verify]

Signerer ein lokal macOS-appbundle.

- utan --identity bruker skriptet ad-hoc-signering ("-")
- med --identity kan du bruke Developer ID Application eller anna lokal codesign-identitet
- med --verify køyrer skriptet verifikasjon etter signering
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
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

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_MACOS_SIGN_APP="$APP_PATH"
export NORSCODE_MACOS_SIGN_IDENTITY="$IDENTITY"
export NORSCODE_MACOS_SIGN_ENTITLEMENTS="$ENTITLEMENTS"
export NORSCODE_MACOS_SIGN_VERIFY="$VERIFY"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT_DIR" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/sign-macos-app.no"
