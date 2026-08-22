#!/usr/bin/env sh
# Norscode-first wrapper: Linux app-pakking ligg i tools/package-linux-app.no.
# Shell-delen under er berre avgrensa reserveveg når runtime manglar exec_prosess.
set -eu

usage() {
  cat >&2 <<'EOF'
bruk: nc package-linux-app [--version VER] [--format appdir|tarball|appimage|all]

Byggjer første Linux app-/pakke-artefakt rundt den lokale Norscode-runtime-en.

- `appdir`: lagar AppDir-struktur under build/linux-app/
- `tarball`: pakkar AppDir som .tar.gz
- `appimage`: prøver å bygge AppImage dersom appimagetool finst
- `all`: appdir + tarball + appimage (beste innsats)
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
FORMAT="all"
VERSION=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --version)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      VERSION="$2"
      shift 2
      ;;
    --format)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      FORMAT="$2"
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

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_LINUX_PACKAGE_FORMAT="$FORMAT"
export NORSCODE_LINUX_PACKAGE_VERSION="$VERSION"

OUT="${TMPDIR:-/tmp}/norscode_package_linux_app_$$.log"
rc=0

print_file() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    printf '%s\n' "$_line"
  done < "$_file"
}

has_exec_gap() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"Ukjent funksjon: exec_prosess"*|*"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

"$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/package-linux-app.no" >"$OUT" 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then
  print_file "$OUT"
  rm -f "$OUT"
  exit 0
fi
if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  rm -f "$OUT"
  exit "$rc"
fi
rm -f "$OUT"
printf 'Merk: brukar avgrensa shell-reserveveg fordi runtime manglar exec_prosess i package-linux-app.no.\n' >&2

if [ ! -x "$ROOT_DIR/dist/norscode_native" ]; then
  "$ROOT_DIR/bin/nc" fetch-stage0-seed
fi

if [ "$VERSION" = "" ]; then
  VERSION="1.0"
fi

release_dir="$ROOT_DIR/release-artifacts"
build_dir="$ROOT_DIR/build/linux-app"
appdir="$build_dir/Norscode.AppDir"
tarball="$release_dir/Norscode-linux-$VERSION-AppDir.tar.gz"
appimage_path="$release_dir/Norscode-linux-$VERSION.AppImage"

build_appdir() {
  rm -rf "$appdir"
  mkdir -p "$appdir/usr/bin" "$appdir/usr/share/applications" "$appdir/usr/share/icons/hicolor/256x256/apps" "$appdir/usr/share/norscode/runtime"
  cp "$ROOT_DIR/dist/norscode_native" "$appdir/usr/bin/norscode"
  chmod +x "$appdir/usr/bin/norscode"
  cat > "$appdir/AppRun" <<'EOF'
#!/usr/bin/env sh
set -eu
HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
exec "$HERE/usr/bin/norscode" "$@"
EOF
  chmod +x "$appdir/AppRun"
  cat > "$appdir/Norscode.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Norscode
Exec=AppRun
Icon=norscode
Categories=Development;
Terminal=true
EOF
  cp "$ROOT_DIR/frontend/assets/icons/norscode-mark-256.png" "$appdir/usr/share/icons/hicolor/256x256/apps/norscode.png"
  cp "$ROOT_DIR/frontend/assets/icons/norscode-mark-256.png" "$appdir/norscode.png"
  cp -R "$ROOT_DIR/bin" "$ROOT_DIR/bootstrap" "$ROOT_DIR/dist" "$ROOT_DIR/selfhost" "$ROOT_DIR/std" "$ROOT_DIR/norcode.toml" "$ROOT_DIR/README.md" "$ROOT_DIR/LICENSE" "$appdir/usr/share/norscode/runtime/"
  cp "$appdir/Norscode.desktop" "$appdir/usr/share/applications/Norscode.desktop"
  printf 'Bygde AppDir: %s\n' "$appdir"
}

write_sha256() {
  _target="$1"
  if command -v shasum >/dev/null 2>&1; then
    set -- $(shasum -a 256 "$_target")
  else
    set -- $(sha256sum "$_target")
  fi
  printf '%s\n' "$1" > "$_target.sha256"
}

mkdir -p "$build_dir" "$release_dir"
rm -f "$tarball" "$tarball.sha256" "$appimage_path" "$appimage_path.sha256"

case "$FORMAT" in
  appdir)
    build_appdir
    ;;
  tarball)
    build_appdir
    tar -czf "$tarball" -C "$build_dir" "Norscode.AppDir"
    write_sha256 "$tarball"
    printf 'Bygde Linux AppDir-tarball: %s\n' "$tarball"
    ;;
  appimage)
    build_appdir
    if [ "$(uname -s)" != "Linux" ]; then
      printf 'Merk: AppImage-bygging krev Linux-host.\n'
      exit 1
    fi
    if ! command -v appimagetool >/dev/null 2>&1; then
      printf 'Merk: appimagetool er ikkje installert; hoppar over AppImage.\n'
      exit 1
    fi
    ARCH="${ARCH:-x86_64}" appimagetool "$appdir" "$appimage_path" >/dev/null
    write_sha256 "$appimage_path"
    printf 'Bygde AppImage: %s\n' "$appimage_path"
    ;;
  all)
    build_appdir
    tar -czf "$tarball" -C "$build_dir" "Norscode.AppDir"
    write_sha256 "$tarball"
    printf 'Bygde Linux AppDir-tarball: %s\n' "$tarball"
    if [ "$(uname -s)" = "Linux" ] && command -v appimagetool >/dev/null 2>&1; then
      ARCH="${ARCH:-x86_64}" appimagetool "$appdir" "$appimage_path" >/dev/null
      write_sha256 "$appimage_path"
      printf 'Bygde AppImage: %s\n' "$appimage_path"
    else
      printf 'Merk: AppImage-bygging krev Linux-host og appimagetool; hoppar over AppImage.\n'
    fi
    ;;
  *)
    usage
    exit 1
    ;;
esac
