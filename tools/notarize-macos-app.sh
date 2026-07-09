#!/usr/bin/env sh
# Norscode-first wrapper: macOS notarization ligg i tools/notarize-macos-app.no.
# Shell-delen under set berre rot/prosessmiljø/argument og startar Norscode-eigarfil.
set -eu

usage() {
  cat >&2 <<'EOF'
bruk: nc notarize-macos-app <artefakt.zip|artefakt.dmg|artefakt.pkg>

Notariserer eit macOS-artefakt med xcrun notarytool.
Krev desse miljøvariablane:
  APPLE_ID
  APPLE_TEAM_ID
  APPLE_APP_PASSWORD
EOF
}

if [ "$#" -ne 1 ]; then
  usage
  exit 1
fi

ARTIFACT="$1"

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
case "$ARTIFACT" in
  /*) ;;
  *) ARTIFACT="$ROOT_DIR/$ARTIFACT" ;;
esac

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_MACOS_NOTARIZE_ARTIFACT="$ARTIFACT"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT_DIR" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/notarize-macos-app.no"
