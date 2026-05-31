#!/usr/bin/env sh
# tools/install.sh - Installer Norscode native binary
#
# Oppdager plattform, laster ned riktig pre-bygd binary fra GitHub Releases
# og legger den på PATH.
#
# Bruk:
#   curl -fsSL https://raw.githubusercontent.com/rfwwp8k542-maker/Norscode/main/tools/install.sh | sh
#   bash tools/install.sh               # fra lokal kildekode
#   bash tools/install.sh --prefix /usr/local
set -eu

REPO="rfwwp8k542-maker/Norscode"
INSTALL_PREFIX="${1:-$HOME/.local}"
BIN_DIR="$INSTALL_PREFIX/bin"

# ─── Oppdag plattform ────────────────────────────────────────────────────────
OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Darwin)
        case "$ARCH" in
            arm64)  PLATFORM="macos-arm64" ;;
            x86_64) PLATFORM="macos-x86_64" ;;
            *)      printf 'Ukjent macOS-arkitektur: %s\n' "$ARCH" >&2; exit 1 ;;
        esac
        ;;
    Linux)
        case "$ARCH" in
            x86_64)  PLATFORM="linux-x86_64" ;;
            aarch64) PLATFORM="linux-arm64" ;;
            *)       printf 'Ukjent Linux-arkitektur: %s\n' "$ARCH" >&2; exit 1 ;;
        esac
        ;;
    *)
        printf 'Ikke støttet plattform: %s\n' "$OS" >&2
        printf 'På Windows, bruk: tools\\install.ps1\n' >&2
        exit 1
        ;;
esac

printf 'Plattform oppdaget: %s\n' "$PLATFORM"

# ─── Sjekk om pre-bygd binary allerede finnes lokalt ────────────────────────
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"

# Bruk norscode_native om tilgjengeleg lokalt
if [ -x "$ROOT_DIR/dist/norscode_native" ]; then
    printf 'Pre-bygd norscode_native funnet lokalt.\n'
    mkdir -p "$BIN_DIR"
    cp "$ROOT_DIR/dist/norscode_native" "$BIN_DIR/norscode"
    cp "$ROOT_DIR/dist/norscode_native" "$BIN_DIR/nc"
    chmod +x "$BIN_DIR/norscode" "$BIN_DIR/nc"
    printf 'Installert: %s/norscode\n' "$BIN_DIR"
    printf 'Legg til PATH: export PATH="%s:$PATH"\n' "$BIN_DIR"
    exit 0
fi

# ─── Hent siste release fra GitHub ──────────────────────────────────────────
if command -v curl >/dev/null 2>&1; then
    DOWNLOAD="curl -fsSL"
elif command -v wget >/dev/null 2>&1; then
    DOWNLOAD="wget -qO-"
else
    printf 'Feil: curl eller wget er påkrevd.\n' >&2
    exit 1
fi

RELEASES_URL="https://api.github.com/repos/$REPO/releases/latest"
printf 'Henter siste release...\n'

RELEASE_JSON="$($DOWNLOAD "$RELEASES_URL" 2>/dev/null)" || {
    printf 'Feil: Kunne ikke hente release-info fra GitHub.\n' >&2
    printf 'Prøv å bygge lokalt: bash tools/build-bootstrap-binary.sh\n' >&2
    exit 1
}

# Finn riktig binary-URL for denne plattformen
BINARY_NAME="norscode-$PLATFORM"
DOWNLOAD_URL="$(printf '%s' "$RELEASE_JSON" | grep -o "\"browser_download_url\": \"[^\"]*$BINARY_NAME[^\"]*\"" | head -1 | cut -d'"' -f4)"

if [ -z "$DOWNLOAD_URL" ]; then
    printf 'Advarsel: Ingen pre-bygd binary for %s i siste release.\n' "$PLATFORM" >&2
    printf 'Feil: Kunne ikke installere Norscode for %s.\n' "$PLATFORM" >&2
    printf 'Bygg lokalt med: bash tools/build-bootstrap-binary.sh\n' >&2
    exit 1
fi

# ─── Last ned og installer ───────────────────────────────────────────────────
mkdir -p "$BIN_DIR"
TEMP_FILE="$(mktemp)"
printf 'Laster ned %s...\n' "$BINARY_NAME"
$DOWNLOAD "$DOWNLOAD_URL" > "$TEMP_FILE"
mv "$TEMP_FILE" "$BIN_DIR/norscode"
chmod +x "$BIN_DIR/norscode"

# Lag nc-alias
ln -sf "$BIN_DIR/norscode" "$BIN_DIR/nc" 2>/dev/null || cp "$BIN_DIR/norscode" "$BIN_DIR/nc"

printf '\nNorscode installert!\n'
printf '  Binary: %s/norscode\n' "$BIN_DIR"
printf '  Alias:  %s/nc\n' "$BIN_DIR"
printf '\nLegg til PATH (bash/zsh):\n'
printf '  export PATH="%s:$PATH"\n' "$BIN_DIR"
printf '\nTest:\n'
printf '  norscode --help\n'
