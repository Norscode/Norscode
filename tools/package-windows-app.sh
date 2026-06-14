#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/package-windows-app.sh [--version VER] [EXE_PATH]

Pakkar ein Windows-native norscode.exe til release-artifacts/ med ZIP og SHA256.
Skriptet byggjer ikkje .exe-en sjølv; det standardiserer release-kontrakten rundt ein eksisterande artefakt og legg han inn i ein enkel app-layout.
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
RELEASE_DIR="$ROOT_DIR/release-artifacts"
EXE_PATH=""

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

if [ -z "$EXE_PATH" ]; then
  printf 'Feil: manglar path til Windows-native .exe\n' >&2
  printf 'Døme: bash tools/package-windows-app.sh build/windows/norscode.exe\n' >&2
  exit 1
fi

case "$EXE_PATH" in
  /*) ;;
  *) EXE_PATH="$ROOT_DIR/$EXE_PATH" ;;
esac

if [ ! -f "$EXE_PATH" ]; then
  printf 'Feil: fann ikkje .exe-artefakt: %s\n' "$EXE_PATH" >&2
  exit 1
fi

if ! command -v zip >/dev/null 2>&1; then
  printf 'Feil: manglar zip-verktøyet\n' >&2
  exit 1
fi

mkdir -p "$RELEASE_DIR"
STAGE_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGE_DIR"' EXIT

PKG_NAME="norscode-windows-${VERSION}"
ZIP_PATH="$RELEASE_DIR/${PKG_NAME}.zip"
LAYOUT_ROOT="$STAGE_DIR/layout"

bash "$ROOT_DIR/tools/build-windows-app-layout.sh" --out "$LAYOUT_ROOT" "$EXE_PATH" >/dev/null

rm -f "$ZIP_PATH" "$ZIP_PATH.sha256"
(
  cd "$LAYOUT_ROOT"
  zip -qr "$ZIP_PATH" "Norscode"
)
shasum -a 256 "$ZIP_PATH" | awk '{print $1}' > "$ZIP_PATH.sha256"

printf 'Bygde Windows ZIP-artefakt: %s\n' "$ZIP_PATH"
printf 'SHA256: %s\n' "$ZIP_PATH.sha256"
