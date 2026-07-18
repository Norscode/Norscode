#!/usr/bin/env sh
# Norscode-first wrapper: macOS app-bygg ligg i tools/build-macos-app.no.
# Shell-delen under set berre rot/macOS-miljø/argument og startar Norscode-eigarfil.
set -eu

usage() {
  cat >&2 <<'EOF'
bruk: nc build-macos-app [--config FIL] [--mode gui|terminal] [--name NAMN] [--out DIR] [--bundle-id ID] [--version VER]

Lagar ein minimal, usignert macOS .app-bundle rundt den lokale Norscode-runtime-en.
Standard output: build/macos-app/Norscode.app
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
APP_NAME="Norscode"
OUT_DIR="$ROOT_DIR/build/macos-app"
BUNDLE_ID="dev.norscode.app"
APP_VERSION=""
CONFIG_FILE=""
ICON_PATH="$ROOT_DIR/frontend/assets/icons/norscode.icns"
DOC_TYPE_SOURCE_NAME="Norscode Source"
DOC_TYPE_UTILITY_NAME="Norscode Utility Source"
DOC_TYPE_BYTECODE_NAME="Norscode Bytecode JSON"
WELCOME_TITLE="Norscode macOS app-bundle"
WELCOME_MESSAGE="Velkomen til Norscode på macOS."
APP_MODE="gui"

load_config() {
  config_path="$1"
  if [ ! -f "$config_path" ]; then
    printf 'Feil: manglar config-fil: %s\n' "$config_path" >&2
    exit 1
  fi
  # shellcheck disable=SC1090
  . "$config_path"
}

parse_config_flag() {
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --config)
        [ "$#" -ge 2 ] || { usage; exit 1; }
        CONFIG_FILE="$2"
        return 0
        ;;
    esac
    shift
  done
}

parse_config_flag "$@"

if [ -n "$CONFIG_FILE" ]; then
  case "$CONFIG_FILE" in
    /*) ;;
    *) CONFIG_FILE="$ROOT_DIR/$CONFIG_FILE" ;;
  esac
  load_config "$CONFIG_FILE"
fi

while [ "$#" -gt 0 ]; do
  case "$1" in
    --config)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      shift 2
      ;;
    --name)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_NAME="$2"
      shift 2
      ;;
    --mode)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_MODE="$2"
      shift 2
      ;;
    --out)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      OUT_DIR="$2"
      shift 2
      ;;
    --bundle-id)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      BUNDLE_ID="$2"
      shift 2
      ;;
    --version)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_VERSION="$2"
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

if [ "$(uname -s)" != "Darwin" ]; then
  printf 'Feil: build-macos-app krev macOS.\n' >&2
  exit 1
fi

if [ ! -x "$ROOT_DIR/bin/nc" ]; then
  printf 'Feil: manglar %s/bin/nc\n' "$ROOT_DIR" >&2
  exit 1
fi

if [ ! -x "$ROOT_DIR/dist/norscode_native" ]; then
  printf 'Feil: manglar dist/norscode_native. Køyr: ./bin/nc fetch-stage0-seed\n' >&2
  exit 1
fi

if ! command -v plutil >/dev/null 2>&1; then
  printf 'Feil: manglar macOS-verktøyet plutil\n' >&2
  exit 1
fi

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_HOST_OS="$(uname -s)"
export NORSCODE_SWIFTC="$(command -v swiftc || true)"
export NORSCODE_MACOS_APP_NAME="$APP_NAME"
export NORSCODE_MACOS_APP_OUT="$OUT_DIR"
export NORSCODE_MACOS_APP_BUNDLE_ID="$BUNDLE_ID"
export NORSCODE_MACOS_APP_VERSION="$APP_VERSION"
export NORSCODE_MACOS_APP_ICON="$ICON_PATH"
export NORSCODE_MACOS_APP_MODE="$APP_MODE"
export NORSCODE_MACOS_DOC_SOURCE_NAME="$DOC_TYPE_SOURCE_NAME"
export NORSCODE_MACOS_DOC_UTILITY_NAME="$DOC_TYPE_UTILITY_NAME"
export NORSCODE_MACOS_DOC_BYTECODE_NAME="$DOC_TYPE_BYTECODE_NAME"
export NORSCODE_MACOS_WELCOME_TITLE="$WELCOME_TITLE"
export NORSCODE_MACOS_WELCOME_MESSAGE="$WELCOME_MESSAGE"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT_DIR" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/build-macos-app.no"
