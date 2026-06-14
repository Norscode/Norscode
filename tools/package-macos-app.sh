#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/package-macos-app.sh [--version VER] [--format dmg|zip|pkg|all] [APP_PATH]

Pakkar den lokale Norscode.app til release-artifacts/ som macOS-artefakt.
- default format: all
- byggjer appen automatisk dersom APP_PATH ikkje er gitt
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP_PATH="$ROOT_DIR/build/macos-app/Norscode.app"
RELEASE_DIR="$ROOT_DIR/release-artifacts"
FORMAT="all"

read_repo_version() {
  local toml="$ROOT_DIR/norcode.toml"
  if [ -f "$toml" ]; then
    grep '^version' "$toml" | head -1 | sed 's/.*= *"\(.*\)"/\1/'
  else
    printf '1.0'
  fi
}

VERSION="$(read_repo_version)"

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

if [ "$(uname -s)" != "Darwin" ]; then
  printf 'Feil: package-macos-app krev macOS.\n' >&2
  exit 1
fi

for cmd in hdiutil ditto plutil pkgbuild; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    printf 'Feil: manglar macOS-verktøyet %s\n' "$cmd" >&2
    exit 1
  fi
done

if [ ! -d "$APP_PATH" ]; then
  bash "$ROOT_DIR/tools/build-macos-app.sh" --mode gui --version "$VERSION"
fi

if [ ! -d "$APP_PATH" ]; then
  printf 'Feil: manglar app-bundle: %s\n' "$APP_PATH" >&2
  exit 1
fi

plutil -lint "$APP_PATH/Contents/Info.plist" >/dev/null

mkdir -p "$RELEASE_DIR"
APP_BASENAME="$(basename "$APP_PATH" .app)"
ZIP_PATH="$RELEASE_DIR/${APP_BASENAME}-macos-${VERSION}.zip"
DMG_PATH="$RELEASE_DIR/${APP_BASENAME}-macos-${VERSION}.dmg"
PKG_PATH="$RELEASE_DIR/${APP_BASENAME}-macos-${VERSION}.pkg"
STAGE_DIR="$(mktemp -d)"
DMG_SRC_DIR="$STAGE_DIR/dmg-root"
PKG_ROOT_DIR="$STAGE_DIR/pkg-root"
VOL_NAME="${APP_BASENAME} ${VERSION}"
TMP_DMG="$STAGE_DIR/${APP_BASENAME}.dmg"
trap 'rm -rf "$STAGE_DIR"' EXIT

rm -f "$ZIP_PATH" "$ZIP_PATH.sha256" "$DMG_PATH" "$DMG_PATH.sha256" "$PKG_PATH" "$PKG_PATH.sha256"

package_zip() {
  ditto -c -k --sequesterRsrc --keepParent "$APP_PATH" "$ZIP_PATH"
  shasum -a 256 "$ZIP_PATH" | awk '{print $1}' > "$ZIP_PATH.sha256"
  printf 'Bygde ZIP-artefakt: %s\n' "$ZIP_PATH"
}

package_dmg() {
  mkdir -p "$DMG_SRC_DIR"
  cp -R "$APP_PATH" "$DMG_SRC_DIR/"
  ln -s /Applications "$DMG_SRC_DIR/Applications"
  if ! hdiutil create -volname "$VOL_NAME" -srcfolder "$DMG_SRC_DIR" -ov -format UDZO "$TMP_DMG" >/dev/null; then
    printf 'Merk: DMG-bygging feila via hdiutil, ZIP-artefakt kan framleis brukast.\n' >&2
    return 1
  fi
  mv "$TMP_DMG" "$DMG_PATH"
  shasum -a 256 "$DMG_PATH" | awk '{print $1}' > "$DMG_PATH.sha256"
  printf 'Bygde DMG-artefakt: %s\n' "$DMG_PATH"
}

package_pkg() {
  local install_root="$PKG_ROOT_DIR/Applications"
  mkdir -p "$install_root"
  cp -R "$APP_PATH" "$install_root/"
  pkgbuild \
    --root "$PKG_ROOT_DIR" \
    --identifier "dev.norscode.app.pkg" \
    --version "$VERSION" \
    --install-location "/" \
    "$PKG_PATH" >/dev/null
  shasum -a 256 "$PKG_PATH" | awk '{print $1}' > "$PKG_PATH.sha256"
  printf 'Bygde PKG-artefakt: %s\n' "$PKG_PATH"
}

case "$FORMAT" in
  zip) package_zip ;;
  dmg) package_dmg ;;
  pkg) package_pkg ;;
  both|all)
    package_zip
    package_dmg || true
    package_pkg
    ;;
  *)
    printf 'Feil: ugyldig format %s (bruk zip, dmg, pkg eller all)\n' "$FORMAT" >&2
    exit 1
    ;;
esac
