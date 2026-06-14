#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/install-linux-appdir.sh <Norscode-linux-*-AppDir.tar.gz> [--prefix DIR]

Installerer eller oppgraderer Linux AppDir-artefakt lokalt.
Standard prefix: ~/.local/opt/norscode-app
EOF
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ARCHIVE_PATH="$1"
shift
PREFIX="$HOME/.local/opt/norscode-app"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      PREFIX="$2"
      shift 2
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

case "$ARCHIVE_PATH" in
  /*) ;;
  *) ARCHIVE_PATH="$ROOT_DIR/$ARCHIVE_PATH" ;;
esac

case "$PREFIX" in
  /*) ;;
  *) PREFIX="$ROOT_DIR/$PREFIX" ;;
esac

if [ ! -f "$ARCHIVE_PATH" ]; then
  printf 'Feil: manglar Linux AppDir-tarball: %s\n' "$ARCHIVE_PATH" >&2
  exit 1
fi

if [ -f "${ARCHIVE_PATH}.sha256" ]; then
  EXPECTED_SHA="$(cat "${ARCHIVE_PATH}.sha256")"
  ACTUAL_SHA="$(shasum -a 256 "$ARCHIVE_PATH" | awk '{print $1}')"
  if [ "$EXPECTED_SHA" != "$ACTUAL_SHA" ]; then
    printf 'Feil: SHA256-avvik for %s\n' "$ARCHIVE_PATH" >&2
    exit 1
  fi
fi

VERSION_NAME="$(basename "$ARCHIVE_PATH" .tar.gz)"
INSTALL_ROOT="$PREFIX/versions"
TARGET_DIR="$INSTALL_ROOT/$VERSION_NAME"
CURRENT_LINK="$PREFIX/current"
BIN_DIR="$PREFIX/bin"
STAGE_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGE_DIR"' EXIT

mkdir -p "$INSTALL_ROOT" "$BIN_DIR"
rm -rf "$TARGET_DIR"
mkdir -p "$TARGET_DIR"

tar -xzf "$ARCHIVE_PATH" -C "$STAGE_DIR"
APPDIR_NAME="$(find "$STAGE_DIR" -maxdepth 1 -type d -name '*.AppDir' | head -1)"

if [ -z "${APPDIR_NAME:-}" ]; then
  printf 'Feil: fann ikkje AppDir i tarballen\n' >&2
  exit 1
fi

cp -R "$APPDIR_NAME"/. "$TARGET_DIR"/
ln -sfn "$TARGET_DIR" "$CURRENT_LINK"
ln -sfn "$CURRENT_LINK/AppRun" "$BIN_DIR/norscode-app"

printf 'Installert Linux AppDir: %s\n' "$TARGET_DIR"
printf 'Aktiv lenke: %s\n' "$CURRENT_LINK"
printf 'Launcher: %s\n' "$BIN_DIR/norscode-app"
