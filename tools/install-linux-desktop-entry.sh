#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/install-linux-desktop-entry.sh [APPDIR] [--prefix DIR]

Installerer Linux desktop entry og ikon for Norscode frå ein bygd AppDir.
Standard prefix: ~/.local/share
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APPDIR="$ROOT_DIR/build/linux-app/Norscode.AppDir"
PREFIX="$HOME/.local/share"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      PREFIX="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      if [ "$APPDIR" = "$ROOT_DIR/build/linux-app/Norscode.AppDir" ]; then
        APPDIR="$1"
        shift
      else
        usage
        exit 1
      fi
      ;;
  esac
done

case "$APPDIR" in
  /*) ;;
  *) APPDIR="$ROOT_DIR/$APPDIR" ;;
esac

case "$PREFIX" in
  /*) ;;
  *) PREFIX="$ROOT_DIR/$PREFIX" ;;
esac

DESKTOP_SRC="$APPDIR/usr/share/applications/Norscode.desktop"
ICON_SRC="$APPDIR/usr/share/icons/hicolor/256x256/apps/norscode.png"
DESKTOP_DST_DIR="$PREFIX/applications"
ICON_DST_DIR="$PREFIX/icons/hicolor/256x256/apps"
DESKTOP_DST="$DESKTOP_DST_DIR/Norscode.desktop"
ICON_DST="$ICON_DST_DIR/norscode.png"

if [ ! -f "$DESKTOP_SRC" ]; then
  printf 'Feil: manglar desktop-fil: %s\n' "$DESKTOP_SRC" >&2
  printf 'Bygg først med: bash tools/package-linux-app.sh --format appdir\n' >&2
  exit 1
fi

if [ ! -f "$ICON_SRC" ]; then
  printf 'Feil: manglar ikonfil: %s\n' "$ICON_SRC" >&2
  exit 1
fi

mkdir -p "$DESKTOP_DST_DIR" "$ICON_DST_DIR"
cp "$ICON_SRC" "$ICON_DST"
sed "s|^Exec=.*$|Exec=$APPDIR/AppRun|" "$DESKTOP_SRC" > "$DESKTOP_DST"

if command -v desktop-file-validate >/dev/null 2>&1; then
  desktop-file-validate "$DESKTOP_DST" || true
fi

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$DESKTOP_DST_DIR" || true
fi

printf 'Installert desktop entry: %s\n' "$DESKTOP_DST"
printf 'Installert ikon: %s\n' "$ICON_DST"
