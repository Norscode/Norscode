#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/package-linux-app.sh [--version VER] [--format appdir|tarball|appimage|all]

Byggjer første Linux app-/pakke-artefakt rundt den lokale Norscode-runtime-en.

- `appdir`: lagar AppDir-struktur under build/linux-app/
- `tarball`: pakkar AppDir som .tar.gz
- `appimage`: prøver å bygge AppImage dersom appimagetool finst
- `all`: appdir + tarball + appimage (beste innsats)
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
FORMAT="all"
RELEASE_DIR="$ROOT_DIR/release-artifacts"
BUILD_DIR="$ROOT_DIR/build/linux-app"
APPDIR="$BUILD_DIR/Norscode.AppDir"

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
      usage
      exit 1
      ;;
  esac
done

mkdir -p "$BUILD_DIR" "$RELEASE_DIR"

if [ ! -x "$ROOT_DIR/dist/norscode_native" ]; then
  bash "$ROOT_DIR/tools/build_norscode_native.sh"
fi

APPDIR_TARBALL="$RELEASE_DIR/Norscode-linux-${VERSION}-AppDir.tar.gz"
APPIMAGE_PATH="$RELEASE_DIR/Norscode-linux-${VERSION}.AppImage"

rm -rf "$APPDIR"
rm -f "$APPDIR_TARBALL" "$APPDIR_TARBALL.sha256" "$APPIMAGE_PATH" "$APPIMAGE_PATH.sha256"

build_appdir() {
  mkdir -p \
    "$APPDIR/usr/bin" \
    "$APPDIR/usr/share/applications" \
    "$APPDIR/usr/share/icons/hicolor/256x256/apps" \
    "$APPDIR/usr/share/norscode/runtime"

  cp "$ROOT_DIR/dist/norscode_native" "$APPDIR/usr/bin/norscode"
  chmod +x "$APPDIR/usr/bin/norscode"

  cat > "$APPDIR/AppRun" <<'EOF'
#!/usr/bin/env sh
set -eu
HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
exec "$HERE/usr/bin/norscode" "$@"
EOF
  chmod +x "$APPDIR/AppRun"

  cat > "$APPDIR/Norscode.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Norscode
Exec=AppRun
Icon=norscode
Categories=Development;
Terminal=true
EOF

  cp "$ROOT_DIR/frontend/assets/icons/norscode-mark-256.png" \
    "$APPDIR/usr/share/icons/hicolor/256x256/apps/norscode.png"
  cp "$ROOT_DIR/frontend/assets/icons/norscode-mark-256.png" "$APPDIR/norscode.png"

  cp -R \
    "$ROOT_DIR/bin" \
    "$ROOT_DIR/bootstrap" \
    "$ROOT_DIR/dist" \
    "$ROOT_DIR/selfhost" \
    "$ROOT_DIR/std" \
    "$ROOT_DIR/norcode.toml" \
    "$ROOT_DIR/README.md" \
    "$ROOT_DIR/LICENSE" \
    "$APPDIR/usr/share/norscode/runtime/"

  cp "$APPDIR/Norscode.desktop" "$APPDIR/usr/share/applications/Norscode.desktop"
  printf 'Bygde AppDir: %s\n' "$APPDIR"
}

package_tarball() {
  tar -czf "$APPDIR_TARBALL" -C "$BUILD_DIR" "$(basename "$APPDIR")"
  shasum -a 256 "$APPDIR_TARBALL" | awk '{print $1}' > "$APPDIR_TARBALL.sha256"
  printf 'Bygde Linux AppDir-tarball: %s\n' "$APPDIR_TARBALL"
}

package_appimage() {
  if [ "$(uname -s)" != "Linux" ]; then
    printf 'Merk: AppImage-bygging krev Linux-host.\n' >&2
    return 1
  fi
  if ! command -v appimagetool >/dev/null 2>&1; then
    printf 'Merk: appimagetool er ikkje installert; hoppar over AppImage.\n' >&2
    return 1
  fi
  ARCH="${ARCH:-x86_64}" appimagetool "$APPDIR" "$APPIMAGE_PATH" >/dev/null
  shasum -a 256 "$APPIMAGE_PATH" | awk '{print $1}' > "$APPIMAGE_PATH.sha256"
  printf 'Bygde AppImage: %s\n' "$APPIMAGE_PATH"
}

case "$FORMAT" in
  appdir)
    build_appdir
    ;;
  tarball)
    build_appdir
    package_tarball
    ;;
  appimage)
    build_appdir
    package_appimage
    ;;
  all)
    build_appdir
    package_tarball
    package_appimage || true
    ;;
  *)
    printf 'Feil: ugyldig format %s (bruk appdir, tarball, appimage eller all)\n' "$FORMAT" >&2
    exit 1
    ;;
esac
