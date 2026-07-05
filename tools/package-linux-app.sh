#!/usr/bin/env sh
# Tynn wrapper: Linux app-pakking ligg i tools/package-linux-app.no.
set -eu

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

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
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
      usage
      exit 1
      ;;
  esac
done

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_LINUX_PACKAGE_FORMAT="$FORMAT"
export NORSCODE_LINUX_PACKAGE_VERSION="$VERSION"

exec "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/package-linux-app.no"
