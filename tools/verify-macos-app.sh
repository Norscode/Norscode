#!/usr/bin/env sh
# Norscode-first wrapper: macOS app-verifikasjon ligg i tools/verify-macos-app.no.
# Shell-delen under set berre rot/prosessmiljø/argument og startar Norscode-eigarfil.
set -eu

usage() {
  cat >&2 <<'EOF'
bruk: nc verify-macos-app [APP_PATH]

Verifiserer structure, plist og codesign-status for ein macOS-appbundle.
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
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

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_MACOS_VERIFY_APP="$APP_PATH"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT_DIR" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/verify-macos-app.no"
