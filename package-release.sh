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
STAGING_DIR="$(mktemp -d)"

cleanup() {
  rm -rf "$STAGING_DIR"
}
trap cleanup EXIT

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

RELEASE_ENTRIES=(
  AGENTS.md
  CHANGELOG.md
  INSTALL.md
  LICENSE
  Makefile
  README.md
  ROADMAP.md
  NorsDB
  app.no
  backend
  bin
  bootstrap
  cli
  compiler
  deploy
  dist
  docs
  examples
  frontend
  nc
  nl
  nor
  norcode.lock
  norcode.toml
  packages
  runtime
  scripts
  selfhost
  server
  std
  stdlib
  tests
  toolchain
  tools
  vscode-norscode
)

PACKAGE_ENTRIES=()
for entry in "${RELEASE_ENTRIES[@]}"; do
  if [ -e "$ROOT_DIR/$entry" ]; then
    PACKAGE_ENTRIES+=("$entry")
  fi
done

copy_entry() {
  src="$1"
  dest="$2"
  mkdir -p "$(dirname "$dest")"
  cp -R "$src" "$dest"
}

for entry in "${PACKAGE_ENTRIES[@]}"; do
  copy_entry "$ROOT_DIR/$entry" "$STAGING_DIR/$entry"
done

# Aktiv compile-sti brukar den hybride v9400-bundlen. Ta med berre denne
# minimale build-artefakten, ikkje heile lokale build/. Release skal feile
# tydeleg dersom artefakten manglar, elles blir installert `nc check` broten.
if [ ! -f "$ROOT_DIR/build/v9400/hybrid_compiler_bundle_v9400.ncb.json" ]; then
  printf 'Manglar release-artefakt: build/v9400/hybrid_compiler_bundle_v9400.ncb.json\n' >&2
  printf 'Køyr v9400 bundle-bygg før release, eller legg den spora artefakten tilbake.\n' >&2
  exit 1
fi
copy_entry "$ROOT_DIR/build/v9400" "$STAGING_DIR/build/v9400"
PACKAGE_ENTRIES+=(build)

# Den aktive precompiled importstien prioriterer .nors for std-modular. Release
# må difor vere sjølvforsynt sjølv om kjelda framleis bur som std/socket.no.
if [ -f "$STAGING_DIR/std/socket.no" ] && [ ! -f "$STAGING_DIR/std/socket.nors" ]; then
  cp "$STAGING_DIR/std/socket.no" "$STAGING_DIR/std/socket.nors"
fi

tar -czf "$ARCHIVE_PATH" -C "$STAGING_DIR" "${PACKAGE_ENTRIES[@]}"

if command -v shasum >/dev/null 2>&1; then
  shasum -a 256 "$ARCHIVE_PATH" | awk '{print $1}' > "${ARCHIVE_PATH}.sha256"
elif command -v sha256sum >/dev/null 2>&1; then
  sha256sum "$ARCHIVE_PATH" | awk '{print $1}' > "${ARCHIVE_PATH}.sha256"
else
  printf 'Fant ikkje verktøy for SHA256-sjekksum\n' >&2
  exit 1
fi

printf 'Bygde releasepakke: %s\n' "$ARCHIVE_PATH"
