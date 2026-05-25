#!/usr/bin/env bash
set -euo pipefail

usage() {
  printf 'Bruk: bash package-release.sh <versjon>\n' >&2
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

VERSION="$1"
shift || true

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
DIST_DIR="$ROOT_DIR/dist"
RELEASE_DIR="$ROOT_DIR/release-artifacts"
ARCHIVE_NAME="norscode-language-${VERSION}.tar.gz"
ARCHIVE_PATH="${RELEASE_DIR}/${ARCHIVE_NAME}"

mkdir -p "$RELEASE_DIR"

if [ ! -x "$DIST_DIR/norcode-bootstrap-compile" ]; then
  printf 'Manglar bootstrap-binary: %s\n' "$DIST_DIR/norcode-bootstrap-compile" >&2
  exit 1
fi

if [ ! -x "$DIST_DIR/norscode" ]; then
  printf 'Manglar distribusjonsbinary: %s\n' "$DIST_DIR/norscode" >&2
  exit 1
fi

if [ ! -f "$ROOT_DIR/norcode.toml" ]; then
  printf 'Manglar norcode.toml\n' >&2
  exit 1
fi

rm -f "$ARCHIVE_PATH" "${ARCHIVE_PATH}.sha256"

tar -czf "$ARCHIVE_PATH" -C "$ROOT_DIR" \
  backend \
  bin \
  cli \
  compiler \
  deploy \
  dist \
  docs \
  examples \
  frontend \
  main.py \
  nc \
  nl \
  nor \
  norcode \
  norcode.toml \
  norsklang \
  package-release.sh \
  packages \
  pyproject.toml \
  runtime \
  scripts \
  selfhost \
  std \
  stdlib \
  tests \
  toolchain \
  tools \
  vscode-norscode \
  README.md \
  LICENSE \
  CHANGELOG.md \
  Makefile \
  app.c \
  app.no

if command -v shasum >/dev/null 2>&1; then
  shasum -a 256 "$ARCHIVE_PATH" | awk '{print $1}' > "${ARCHIVE_PATH}.sha256"
elif command -v sha256sum >/dev/null 2>&1; then
  sha256sum "$ARCHIVE_PATH" | awk '{print $1}' > "${ARCHIVE_PATH}.sha256"
else
  printf 'Fant ikkje verktøy for SHA256-sjekksum\n' >&2
  exit 1
fi

printf 'Bygde releasepakke: %s\n' "$ARCHIVE_PATH"
