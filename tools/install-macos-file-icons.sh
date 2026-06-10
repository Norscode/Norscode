#!/usr/bin/env bash
# Register Norscode file type icons with macOS LaunchServices.
#
# This creates a small local app bundle that only carries Info.plist document
# type metadata and icon resources. Finder uses that bundle registration to
# show Norscode icons for known extensions after Norscode is installed.

set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/install-macos-file-icons.sh [--bundle-dir DIR] [--skip-register]

Installerer macOS filtype-ikoner for Norscode:
  .no   Norscode source
  .nc   Norscode utility/source
  .ncf  Norscode config
  .ncd  Norscode data
  .ncp  Norscode package

Merk: macOS registrerer ikoner per filtype/extension. Folder-ikonene i
frontend/assets/icons pakkes som ressurser, men må settes per mappe dersom
de skal brukes på konkrete prosjekt-/pakke-mapper.
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
ICON_DIR="$ROOT_DIR/frontend/assets/icons"
BUNDLE_DIR="$HOME/Applications/Norscode File Types.app"
SKIP_REGISTER=0

while [ "$#" -gt 0 ]; do
  case "$1" in
    --bundle-dir)
      if [ "$#" -lt 2 ]; then
        usage
        exit 1
      fi
      BUNDLE_DIR="$2"
      shift 2
      ;;
    --skip-register)
      SKIP_REGISTER=1
      shift
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

if [ "$(uname -s)" != "Darwin" ]; then
  printf 'Hopp over macOS filtype-ikoner: ikkje macOS.\n'
  exit 0
fi

for _cmd in qlmanage sips iconutil plutil; do
  if ! command -v "$_cmd" >/dev/null 2>&1; then
    printf 'Feil: manglar macOS-verktøyet %s\n' "$_cmd" >&2
    exit 1
  fi
done

if [ ! -d "$ICON_DIR" ]; then
  printf 'Hopp over macOS filtype-ikoner: manglar %s\n' "$ICON_DIR"
  exit 0
fi

make_icns_from_svg() {
  local svg_path="$1"
  local icns_path="$2"
  local tmp_dir base preview iconset png_src size scale name

  if [ ! -f "$svg_path" ]; then
    printf 'Feil: manglar ikonkjelde %s\n' "$svg_path" >&2
    exit 1
  fi

  tmp_dir="$(mktemp -d)"
  base="$(basename "$svg_path")"
  iconset="$tmp_dir/icon.iconset"
  mkdir -p "$iconset"

  if ! qlmanage -t -s 1024 -o "$tmp_dir" "$svg_path" >/dev/null 2>&1; then
    if [ -f "$ICON_DIR/norscode.icns" ]; then
      cp "$ICON_DIR/norscode.icns" "$icns_path"
      rm -rf "$tmp_dir"
      return 0
    fi
    printf 'Feil: klarte ikkje rendre %s med QuickLook\n' "$svg_path" >&2
    rm -rf "$tmp_dir"
    exit 1
  fi
  preview="$tmp_dir/${base}.png"
  if [ ! -f "$preview" ]; then
    preview="$(find "$tmp_dir" -maxdepth 1 -type f -name '*.png' | head -1)"
  fi
  if [ -z "${preview:-}" ] || [ ! -f "$preview" ]; then
    if [ -f "$ICON_DIR/norscode.icns" ]; then
      cp "$ICON_DIR/norscode.icns" "$icns_path"
      rm -rf "$tmp_dir"
      return 0
    fi
    printf 'Feil: klarte ikkje lage PNG-preview frå %s\n' "$svg_path" >&2
    rm -rf "$tmp_dir"
    exit 1
  fi

  for size in 16 32 128 256 512; do
    for scale in 1 2; do
      if [ "$scale" -eq 1 ]; then
        name="icon_${size}x${size}.png"
      else
        name="icon_${size}x${size}@2x.png"
      fi
      png_src="$iconset/$name"
      cp "$preview" "$png_src"
      sips -z "$((size * scale))" "$((size * scale))" "$png_src" >/dev/null
    done
  done

  iconutil -c icns "$iconset" -o "$icns_path"
  rm -rf "$tmp_dir"
}

mkdir -p "$BUNDLE_DIR/Contents/MacOS" "$BUNDLE_DIR/Contents/Resources"

if [ -f "$ICON_DIR/norscode.icns" ]; then
  cp "$ICON_DIR/norscode.icns" "$BUNDLE_DIR/Contents/Resources/norscode.icns"
else
  make_icns_from_svg "$ICON_DIR/norscode-mark.svg" "$BUNDLE_DIR/Contents/Resources/norscode.icns"
fi

make_icns_from_svg "$ICON_DIR/file-main-no.svg" "$BUNDLE_DIR/Contents/Resources/file-main-no.icns"
make_icns_from_svg "$ICON_DIR/file-utils-nc.svg" "$BUNDLE_DIR/Contents/Resources/file-utils-nc.icns"
make_icns_from_svg "$ICON_DIR/file-config-ncf.svg" "$BUNDLE_DIR/Contents/Resources/file-config-ncf.icns"
make_icns_from_svg "$ICON_DIR/file-data-ncd.svg" "$BUNDLE_DIR/Contents/Resources/file-data-ncd.icns"
make_icns_from_svg "$ICON_DIR/file-package-ncp.svg" "$BUNDLE_DIR/Contents/Resources/file-package-ncp.icns"

# Keep these as bundle resources for future per-folder/custom project use.
cp "$ICON_DIR/file-component-no.svg" "$BUNDLE_DIR/Contents/Resources/file-component-no.svg"
cp "$ICON_DIR/folder-project.svg" "$BUNDLE_DIR/Contents/Resources/folder-project.svg"
cp "$ICON_DIR/folder-package.svg" "$BUNDLE_DIR/Contents/Resources/folder-package.svg"

cat > "$BUNDLE_DIR/Contents/MacOS/NorscodeFileTypes" <<'EOF'
#!/usr/bin/env sh
exec /usr/bin/open -a Terminal "$HOME"
EOF
chmod +x "$BUNDLE_DIR/Contents/MacOS/NorscodeFileTypes"

cat > "$BUNDLE_DIR/Contents/Info.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>nb</string>
  <key>CFBundleDisplayName</key>
  <string>Norscode File Types</string>
  <key>CFBundleExecutable</key>
  <string>NorscodeFileTypes</string>
  <key>CFBundleIconFile</key>
  <string>norscode</string>
  <key>CFBundleIdentifier</key>
  <string>dev.norscode.filetypes</string>
  <key>CFBundleName</key>
  <string>Norscode File Types</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>1.0</string>
  <key>CFBundleVersion</key>
  <string>1</string>
  <key>LSApplicationCategoryType</key>
  <string>public.app-category.developer-tools</string>
  <key>LSMinimumSystemVersion</key>
  <string>11.0</string>
  <key>LSUIElement</key>
  <true/>
  <key>CFBundleDocumentTypes</key>
  <array>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>no</string></array>
      <key>CFBundleTypeIconFile</key>
      <string>file-main-no</string>
      <key>CFBundleTypeName</key>
      <string>Norscode Source</string>
      <key>CFBundleTypeRole</key>
      <string>Editor</string>
      <key>LSHandlerRank</key>
      <string>Owner</string>
      <key>LSItemContentTypes</key>
      <array><string>dev.norscode.source</string></array>
    </dict>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>nc</string></array>
      <key>CFBundleTypeIconFile</key>
      <string>file-utils-nc</string>
      <key>CFBundleTypeName</key>
      <string>Norscode Utility Source</string>
      <key>CFBundleTypeRole</key>
      <string>Editor</string>
      <key>LSHandlerRank</key>
      <string>Owner</string>
      <key>LSItemContentTypes</key>
      <array><string>dev.norscode.utility-source</string></array>
    </dict>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>ncf</string></array>
      <key>CFBundleTypeIconFile</key>
      <string>file-config-ncf</string>
      <key>CFBundleTypeName</key>
      <string>Norscode Config</string>
      <key>CFBundleTypeRole</key>
      <string>Editor</string>
      <key>LSHandlerRank</key>
      <string>Owner</string>
      <key>LSItemContentTypes</key>
      <array><string>dev.norscode.config</string></array>
    </dict>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>ncd</string></array>
      <key>CFBundleTypeIconFile</key>
      <string>file-data-ncd</string>
      <key>CFBundleTypeName</key>
      <string>Norscode Data</string>
      <key>CFBundleTypeRole</key>
      <string>Viewer</string>
      <key>LSHandlerRank</key>
      <string>Owner</string>
      <key>LSItemContentTypes</key>
      <array><string>dev.norscode.data</string></array>
    </dict>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>ncp</string></array>
      <key>CFBundleTypeIconFile</key>
      <string>file-package-ncp</string>
      <key>CFBundleTypeName</key>
      <string>Norscode Package</string>
      <key>CFBundleTypeRole</key>
      <string>Editor</string>
      <key>LSHandlerRank</key>
      <string>Owner</string>
      <key>LSItemContentTypes</key>
      <array><string>dev.norscode.package</string></array>
    </dict>
  </array>
  <key>UTExportedTypeDeclarations</key>
  <array>
    <dict>
      <key>UTTypeIdentifier</key>
      <string>dev.norscode.source</string>
      <key>UTTypeDescription</key>
      <string>Norscode source file</string>
      <key>UTTypeConformsTo</key>
      <array><string>public.source-code</string><string>public.text</string></array>
      <key>UTTypeTagSpecification</key>
      <dict>
        <key>public.filename-extension</key>
        <array><string>no</string></array>
      </dict>
    </dict>
    <dict>
      <key>UTTypeIdentifier</key>
      <string>dev.norscode.utility-source</string>
      <key>UTTypeDescription</key>
      <string>Norscode utility source file</string>
      <key>UTTypeConformsTo</key>
      <array><string>public.source-code</string><string>public.text</string></array>
      <key>UTTypeTagSpecification</key>
      <dict>
        <key>public.filename-extension</key>
        <array><string>nc</string></array>
      </dict>
    </dict>
    <dict>
      <key>UTTypeIdentifier</key>
      <string>dev.norscode.config</string>
      <key>UTTypeDescription</key>
      <string>Norscode config file</string>
      <key>UTTypeConformsTo</key>
      <array><string>public.text</string><string>public.data</string></array>
      <key>UTTypeTagSpecification</key>
      <dict>
        <key>public.filename-extension</key>
        <array><string>ncf</string></array>
      </dict>
    </dict>
    <dict>
      <key>UTTypeIdentifier</key>
      <string>dev.norscode.data</string>
      <key>UTTypeDescription</key>
      <string>Norscode data file</string>
      <key>UTTypeConformsTo</key>
      <array><string>public.data</string></array>
      <key>UTTypeTagSpecification</key>
      <dict>
        <key>public.filename-extension</key>
        <array><string>ncd</string></array>
      </dict>
    </dict>
    <dict>
      <key>UTTypeIdentifier</key>
      <string>dev.norscode.package</string>
      <key>UTTypeDescription</key>
      <string>Norscode package file</string>
      <key>UTTypeConformsTo</key>
      <array><string>public.package</string><string>public.data</string></array>
      <key>UTTypeTagSpecification</key>
      <dict>
        <key>public.filename-extension</key>
        <array><string>ncp</string></array>
      </dict>
    </dict>
  </array>
</dict>
</plist>
EOF

plutil -lint "$BUNDLE_DIR/Contents/Info.plist" >/dev/null

if [ "$SKIP_REGISTER" = "0" ]; then
  /System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister \
    -f "$BUNDLE_DIR" >/dev/null 2>&1 || true
fi

printf 'Norscode macOS filtype-ikoner installert: %s\n' "$BUNDLE_DIR"
printf 'Finder kan trenge litt tid, eller relansering, før ikon-cache viser nye ikoner.\n'
