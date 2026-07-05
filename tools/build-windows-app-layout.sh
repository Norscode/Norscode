#!/usr/bin/env sh
# Tynn wrapper: Windows app-layout ligg i tools/build-windows-app-layout.no.
set -eu

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/build-windows-app-layout.sh [--out DIR] [--name NAMN] EXE_PATH

Byggjer ein enkel Windows app-layout rundt ein eksisterande norscode.exe.
Standard output: build/windows-app/Norscode/
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT_ROOT="$ROOT_DIR/build/windows-app"
APP_NAME="Norscode"
EXE_PATH=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --out)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      OUT_ROOT="$2"
      shift 2
      ;;
    --name)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_NAME="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      if [ -z "$EXE_PATH" ]; then
        EXE_PATH="$1"
        shift
      else
        usage
        exit 1
      fi
      ;;
  esac
done

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_WINDOWS_LAYOUT_OUT="$OUT_ROOT"
export NORSCODE_WINDOWS_LAYOUT_NAME="$APP_NAME"
export NORSCODE_WINDOWS_EXE_PATH="$EXE_PATH"

exec "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/build-windows-app-layout.no"
