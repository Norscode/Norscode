#!/usr/bin/env sh
# Register Norscode file type icons with macOS LaunchServices.
set -eu

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/install-macos-file-icons.sh [--bundle-dir DIR] [--skip-register]

Installerer macOS filtype-ikoner for Norscode:
  .no   Norscode source
  .nc   Norscode utility/source
  .ncf  Norscode config
  .ncd  Norscode data
  .ncp  Norscode package

Merk: macOS registrerer ikoner per filtype/extension. Folder-ikonene i
frontend/assets/icons pakkes som ressurser, men må settes per mappe dersom
de skal brukes på konkrete prosjekt-/pakke-mapper.
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
ICON_DIR="$ROOT_DIR/frontend/assets/icons"
BUNDLE_DIR="$HOME/Applications/Norscode File Types.app"
SKIP_REGISTER=usann

while [ "$#" -gt 0 ]; do
  case "$1" in
    --bundle-dir)
      if [ "$#" -lt 2 ]; then
        usage
        exit 1
      fi
      BUNDLE_DIR="$2"
      shift 2
      ;;
    --skip-register)
      SKIP_REGISTER=sann
      shift
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

exec env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_ROOT="$ROOT_DIR" \
  NORSCODE_FILE_ICONS_BUNDLE_DIR="$BUNDLE_DIR" \
  NORSCODE_FILE_ICONS_SKIP_REGISTER="$SKIP_REGISTER" \
  "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/install-macos-file-icons.no"
