#!/usr/bin/env bash
set -euo pipefail

usage() {
  printf 'Bruk: bash tools/install-release.sh <release.tar.gz> [--prefix DIR]\n' >&2
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

ARCHIVE_PATH="$1"
shift

PREFIX="${HOME}/.local/share/norscode"
while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      if [ "$#" -lt 2 ]; then
        usage
        exit 1
      fi
      PREFIX="$2"
      shift 2
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

if [ ! -f "$ARCHIVE_PATH" ]; then
  printf 'Fant ikke releasepakke: %s\n' "$ARCHIVE_PATH" >&2
  exit 1
fi

if [ -f "${ARCHIVE_PATH}.sha256" ]; then
  EXPECTED_SHA256="$(cat "${ARCHIVE_PATH}.sha256")"
  if command -v shasum >/dev/null 2>&1; then
    ACTUAL_SHA256="$(shasum -a 256 "$ARCHIVE_PATH" | awk '{print $1}')"
  elif command -v sha256sum >/dev/null 2>&1; then
    ACTUAL_SHA256="$(sha256sum "$ARCHIVE_PATH" | awk '{print $1}')"
  else
    printf 'Fant ikke verktøy for SHA256-verifisering\n' >&2
    exit 1
  fi
  if [ "$EXPECTED_SHA256" != "$ACTUAL_SHA256" ]; then
    printf 'SHA256-avvik for releasepakken\n' >&2
    exit 1
  fi
fi

INSTALL_ROOT="${PREFIX}/releases"
CURRENT_LINK="${PREFIX}/current"
STAGING_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGING_DIR"' EXIT

mkdir -p "$INSTALL_ROOT"
tar -xzf "$ARCHIVE_PATH" -C "$STAGING_DIR"

if [ ! -x "$STAGING_DIR/bin/nc" ]; then
  printf 'Releasepakken mangler bin/nc\n' >&2
  exit 1
fi

if [ ! -x "$STAGING_DIR/dist/norscode" ]; then
  printf 'Releasepakken mangler dist/norscode\n' >&2
  exit 1
fi

VERSION_NAME="$(basename "$ARCHIVE_PATH" .tar.gz)"
TARGET_DIR="${INSTALL_ROOT}/${VERSION_NAME}"

rm -rf "$TARGET_DIR"
mkdir -p "$TARGET_DIR"
cp -R "$STAGING_DIR"/. "$TARGET_DIR"/

ln -sfn "$TARGET_DIR" "$CURRENT_LINK"
mkdir -p "${PREFIX}/bin"
ln -sfn "${CURRENT_LINK}/bin/nc" "${PREFIX}/bin/nc"
ln -sfn "${CURRENT_LINK}/bin/nor" "${PREFIX}/bin/nor"
ln -sfn "${CURRENT_LINK}/bin/nl" "${PREFIX}/bin/nl"
ln -sfn "${CURRENT_LINK}/bin/bootstrap" "${PREFIX}/bin/bootstrap"

printf 'Installert release: %s\n' "$TARGET_DIR"
printf 'Aktiv versjon: %s\n' "$CURRENT_LINK"
