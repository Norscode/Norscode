#!/usr/bin/env sh
# Norscode-first wrapper: macOS app-pakking ligg i tools/package-macos-app.no.
# Shell-delen under set berre rot/prosessmiljø/argument og startar Norscode-eigarfil.
set -eu

usage() {
  cat >&2 <<'EOF'
bruk: nc package-macos-app [--version VER] [--format dmg|zip|pkg|all] [APP_PATH]

Pakkar den lokale Norscode.app til release-artifacts/ som macOS-artefakt.
- default format: all
- byggjer appen automatisk dersom APP_PATH ikkje er gitt
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
APP_PATH="$ROOT_DIR/build/macos-app/Norscode.app"
FORMAT="all"
VERSION=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --version)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      VERSION="$2"
      shift 2
      ;;
    --format)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      FORMAT="$2"
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

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_HOST_OS="$(uname -s)"
export NORSCODE_MACOS_PACKAGE_APP="$APP_PATH"
export NORSCODE_MACOS_PACKAGE_FORMAT="$FORMAT"
export NORSCODE_MACOS_PACKAGE_VERSION="$VERSION"
export NORSCODE_MACOS_PACKAGE_RELEASE_DIR="${NORSCODE_MACOS_PACKAGE_RELEASE_DIR:-$ROOT_DIR/release-artifacts}"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT_DIR" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/package-macos-app.no"
