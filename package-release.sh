#!/usr/bin/env sh
# Norscode-first wrapper: releasepakke-aliaset ligg i package-release.no og tools/package_release.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu

if [ "$#" -lt 1 ]; then
  printf 'bruk: nc package-release <versjon>\n' >&2
  exit 1
fi

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
out="${TMPDIR:-/tmp}/norscode_package_release_$$.log"
rc=0

print_file() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    printf '%s\n' "$_line"
  done < "$_file"
}

has_release_fallback_reason() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"Ukjent funksjon: exec_prosess"*|\
      *"Ukjent funksjon: builtin.exec_prosess"*|\
      *"Ukjent funksjon: builtin.builtin.exec_prosess"*|\
      *"Manglar distribusjonsbinary: "*/dist/norscode_native*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

first_word() {
  set -- $1
  printf '%s\n' "${1:-}"
}

sha256_for_file() {
  _file="$1"
  if command -v shasum >/dev/null 2>&1; then
    first_word "$(shasum -a 256 "$_file")"
    return 0
  fi
  if command -v sha256sum >/dev/null 2>&1; then
    first_word "$(sha256sum "$_file")"
    return 0
  fi
  return 1
}

env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_RELEASE_VERSION="$1" \
  NORSCODE_ROOT="$ROOT_DIR" \
  "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/package_release.no" >"$out" 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then
  print_file "$out"
  rm -f "$out"
  exit 0
fi
if [ -s "$out" ] && ! has_release_fallback_reason "$out"; then
  print_file "$out"
  rm -f "$out"
  exit "$rc"
fi
rm -f "$out"

version="$1"
dist="$ROOT_DIR/dist"
release_dir="$ROOT_DIR/release-artifacts"
archive_name="norscode-language-$version.tar.gz"
archive_path="$release_dir/$archive_name"

[ -x "$dist/norscode_native" ] || {
  printf 'Manglar distribusjonsbinary: %s\n' "$dist/norscode_native" >&2
  exit 1
}
[ -x "$ROOT_DIR/bin/nc" ] || {
  printf 'Manglar CLI-wrapper: %s\n' "$ROOT_DIR/bin/nc" >&2
  exit 1
}
[ -x "$ROOT_DIR/bootstrap/stage0/norscode-macos-arm64" ] || {
  printf 'Manglar macOS stage0-seed: %s\n' "$ROOT_DIR/bootstrap/stage0/norscode-macos-arm64" >&2
  exit 1
}
[ -x "$ROOT_DIR/bootstrap/stage0/norscode-linux-x86_64" ] || {
  printf 'Manglar Linux stage0-seed: %s\n' "$ROOT_DIR/bootstrap/stage0/norscode-linux-x86_64" >&2
  exit 1
}
[ -f "$ROOT_DIR/norcode.toml" ] || {
  printf 'Manglar norcode.toml\n' >&2
  exit 1
}
[ -f "$ROOT_DIR/build/v9400/hybrid_compiler_bundle_v9400.ncb.json" ] || {
  printf 'Manglar release-artefakt: build/v9400/hybrid_compiler_bundle_v9400.ncb.json\n' >&2
  exit 1
}

mkdir -p "$release_dir"
rm -f "$archive_path" "$archive_path.sha256"
staging="$(mktemp -d)"
cleanup() { rm -rf "$staging"; }
trap cleanup EXIT

entries=".github AGENTS.md CHANGELOG.md INSTALL.md LICENSE Makefile README.md ROADMAP.md NorsDB app.no backend bin bootstrap cli compiler deploy dist docs examples frontend nc nl nor norcode.lock norcode.toml packages runtime scripts selfhost server std stdlib tests toolchain tools vscode-norscode"
package_entries=""
for entry in $entries; do
  if [ -e "$ROOT_DIR/$entry" ]; then
    cp -R "$ROOT_DIR/$entry" "$staging/$entry"
    package_entries="$package_entries $entry"
  fi
done

mkdir -p "$staging/build"
cp -R "$ROOT_DIR/build/v9400" "$staging/build/v9400"
package_entries="$package_entries build"

if [ -d "$staging/std" ]; then
  for f in "$staging"/std/*.no; do
    [ -f "$f" ] || continue
    alias_path="${f%.no}.nors"
    [ -f "$alias_path" ] || cp "$f" "$alias_path"
  done
fi

tar -czf "$archive_path" -C "$staging" $package_entries
if ! sha256_for_file "$archive_path" > "$archive_path.sha256"; then
  printf 'Fant ikkje verktøy for SHA256-sjekksum\n' >&2
  exit 1
fi

printf 'Bygde releasepakke: %s\n' "$archive_path"
