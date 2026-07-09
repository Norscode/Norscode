#!/usr/bin/env sh
# Norscode-first wrapper: Windows-pakking ligg i tools/package-windows-app.no.
# Shell-delen under set berre rot/prosessmiljø/argument og startar Norscode-eigarfil.
set -eu

usage() {
  cat >&2 <<'EOF'
bruk: nc package-windows-app [--version VER] <EXE_PATH>

Pakkar ein Windows-native norscode.exe til release-artifacts/ med ZIP og SHA256.
Skriptet byggjer ikkje .exe-en sjølv; det standardiserer release-kontrakten rundt ein eksisterande artefakt og legg han inn i ein enkel app-layout.
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
EXE_PATH=""
VERSION=""

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

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_WINDOWS_PACKAGE_VERSION="$VERSION"
export NORSCODE_WINDOWS_PACKAGE_EXE="$EXE_PATH"

exec env NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}" NORSCODE_ROOT="$ROOT_DIR" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/package-windows-app.no"
