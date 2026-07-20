#!/usr/bin/env sh
# Norscode-first wrapper: releaseinstallasjonen ligg i tools/install_release.no.
# Shell-delen under er avgrensa reserveveg for arkivutpakking og sjekksum.
set -eu

usage() {
  printf 'bruk: nc install-release <release.tar.gz> [--prefix <mappe>]\n' >&2
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

print_file() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    printf '%s\n' "$_line"
  done < "$_file"
}

has_install_reserve_reason() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"Ukjent funksjon: exec_prosess"*|*"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*|*"Fant ikke releasepakke:"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

first_word() {
  _text="$1"
  set -- $_text
  printf '%s' "${1:-}"
}

first_word_from_file() {
  _file="$1"
  IFS= read -r _line < "$_file" || _line=""
  first_word "$_line"
}

sha256_for_file() {
  _file="$1"
  if command -v shasum >/dev/null 2>&1; then
    _line="$(shasum -a 256 "$_file")"
  elif command -v sha256sum >/dev/null 2>&1; then
    _line="$(sha256sum "$_file")"
  else
    return 1
  fi
  first_word "$_line"
}

if [ "${NORSCODE_INSTALL_FORCE_RESERVE:-0}" != "1" ]; then
  env \
    NORSCODE_ENABLE_EXEC_PROSESS=1 \
    NORSCODE_INSTALL_ARCHIVE="$ARCHIVE_PATH" \
    NORSCODE_INSTALL_PREFIX="$PREFIX" \
    NORSCODE_ROOT="$ROOT_DIR" \
    "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/install_release.no" >"$out" 2>&1 || rc=$?
else
  rc=1
fi
if [ "$rc" -eq 0 ]; then
  print_file "$out"
  rm -f "$out"
  exit 0
fi
if [ -s "$out" ] && ! has_install_reserve_reason "$out"; then
  print_file "$out"
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
  expected="$(first_word_from_file "$ARCHIVE_PATH.sha256")"
  if ! actual="$(sha256_for_file "$ARCHIVE_PATH")"; then
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

for entry in bootstrap compiler dist docs examples norcode norcode.toml selfhost scripts std tests tools README.md LICENSE CHANGELOG.md Makefile app.no; do
  if [ -e "$current_link/$entry" ]; then
    ln -sfn "$current_link/$entry" "$PREFIX/$entry"
  fi
done

if [ "$(uname -s)" = "Darwin" ] && [ -x "$current_link/bin/nc" ]; then
  (cd "$current_link" && "$current_link/bin/nc" install-macos-file-icons) || true
fi

printf 'Installert release: %s\n' "$target_dir"
printf 'Aktiv versjon: %s\n' "$current_link"
