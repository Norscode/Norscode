#!/usr/bin/env sh
set -eu

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

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
out="${TMPDIR:-/tmp}/norscode_install_release_$$.log"
rc=0
env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_INSTALL_ARCHIVE="$ARCHIVE_PATH" \
  NORSCODE_INSTALL_PREFIX="$PREFIX" \
  NORSCODE_ROOT="$ROOT_DIR" \
  "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/install_release.no" >"$out" 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then
  cat "$out"
  rm -f "$out"
  exit 0
fi
if [ -s "$out" ] && ! grep -Eq 'Ukjent funksjon: builtin(\.builtin)?\.exec_prosess|Fant ikke releasepakke:' "$out"; then
  cat "$out"
  rm -f "$out"
  exit "$rc"
fi
rm -f "$out"

if [ -z "$PREFIX" ]; then
  PREFIX="${HOME}/.local/share/norscode"
fi
[ -f "$ARCHIVE_PATH" ] || {
  printf 'Fant ikke releasepakke: %s\n' "$ARCHIVE_PATH" >&2
  exit 1
}
if [ -f "$ARCHIVE_PATH.sha256" ]; then
  expected="$(cat "$ARCHIVE_PATH.sha256" | awk '{print $1}')"
  if command -v shasum >/dev/null 2>&1; then
    actual="$(shasum -a 256 "$ARCHIVE_PATH" | awk '{print $1}')"
  elif command -v sha256sum >/dev/null 2>&1; then
    actual="$(sha256sum "$ARCHIVE_PATH" | awk '{print $1}')"
  else
    printf 'Fant ikke verktøy for SHA256-verifisering\n' >&2
    exit 1
  fi
  [ "$expected" = "$actual" ] || {
    printf 'SHA256-avvik for releasepakken\n' >&2
    exit 1
  }
fi

install_root="$PREFIX/releases"
current_link="$PREFIX/current"
staging="$(mktemp -d)"
cleanup() { rm -rf "$staging"; }
trap cleanup EXIT

mkdir -p "$install_root"
tar -xzf "$ARCHIVE_PATH" -C "$staging"
[ -x "$staging/bin/nc" ] || {
  printf 'Releasepakken mangler bin/nc\n' >&2
  exit 1
}
[ -x "$staging/dist/norscode_native" ] || {
  printf 'Releasepakken mangler dist/norscode_native\n' >&2
  exit 1
}

base="$(basename "$ARCHIVE_PATH" .tar.gz)"
target_dir="$install_root/$base"
rm -rf "$target_dir"
mkdir -p "$target_dir"
cp -R "$staging/." "$target_dir/"
ln -sfn "$target_dir" "$current_link"
mkdir -p "$PREFIX/bin"
ln -sfn "$current_link/bin/nc" "$PREFIX/bin/nc"
ln -sfn "$current_link/bin/nor" "$PREFIX/bin/nor"
ln -sfn "$current_link/bin/nl" "$PREFIX/bin/nl"
ln -sfn "$current_link/bin/bootstrap" "$PREFIX/bin/bootstrap"

for entry in compiler dist docs examples norcode norcode.toml selfhost scripts std tests tools README.md LICENSE CHANGELOG.md Makefile app.no; do
  if [ -e "$current_link/$entry" ]; then
    ln -sfn "$current_link/$entry" "$PREFIX/$entry"
  fi
done

if [ "$(uname -s)" = "Darwin" ] && [ -x "$current_link/tools/install-macos-file-icons.sh" ]; then
  bash "$current_link/tools/install-macos-file-icons.sh" || true
fi

printf 'Installert release: %s\n' "$target_dir"
printf 'Aktiv versjon: %s\n' "$current_link"
