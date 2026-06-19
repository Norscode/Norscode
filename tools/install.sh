#!/usr/bin/env sh
# tools/install.sh - Installer Norscode native binary
#
# Oppdager plattform, laster ned riktig pre-bygd binary fra GitHub Releases
# og legger den på PATH.
#
# Bruk:
#   curl -fsSL https://raw.githubusercontent.com/Norscode/Norscode/main/tools/install.sh | sh
#   bash tools/install.sh               # fra lokal kildekode
#   bash tools/install.sh --prefix /usr/local
set -eu

REPO="Norscode/Norscode"
INSTALL_PREFIX="${1:-$HOME/.local}"
BIN_DIR="$INSTALL_PREFIX/bin"
STAGE0_FALLBACK_TAG="${NORSCODE_STAGE0_RELEASE_TAG:-stage0-bootstrap-20260604}"

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
    if [ "$OS" = "Darwin" ] && [ -x "$ROOT_DIR/tools/install-macos-file-icons.sh" ]; then
        bash "$ROOT_DIR/tools/install-macos-file-icons.sh" || true
    fi
    printf 'Installert: %s/norscode\n' "$BIN_DIR"
    printf 'Legg til PATH: export PATH="%s:$PATH"\n' "$BIN_DIR"
    exit 0
fi

# ─── Hent release fra GitHub ────────────────────────────────────────────────
if command -v curl >/dev/null 2>&1; then
    DOWNLOAD="curl -fsSL"
elif command -v wget >/dev/null 2>&1; then
    DOWNLOAD="wget -qO-"
else
    printf 'Feil: curl eller wget er påkrevd.\n' >&2
    exit 1
fi

BINARY_NAME="norscode-$PLATFORM"

find_asset_url() {
    printf '%s' "$1" | grep -o "\"browser_download_url\":[[:space:]]*\"[^\"]*$BINARY_NAME[^\"]*\"" | head -1 | cut -d'"' -f4
}

fetch_release_json() {
    if [ "$1" = "latest" ]; then
        $DOWNLOAD "https://api.github.com/repos/$REPO/releases/latest" 2>/dev/null
    else
        $DOWNLOAD "https://api.github.com/repos/$REPO/releases/tags/$1" 2>/dev/null
    fi
}

printf 'Henter siste release...\n'
RELEASE_JSON="$(fetch_release_json latest)" || {
    printf 'Feil: Kunne ikke hente release-info fra GitHub.\n' >&2
    printf 'Prøv lokal installasjon fra repo: bash tools/install.sh\n' >&2
    exit 1
}

DOWNLOAD_URL="$(find_asset_url "$RELEASE_JSON")"

if [ -z "$DOWNLOAD_URL" ]; then
    printf 'Fant ingen %s i siste release; prøver stage0-release %s...\n' "$BINARY_NAME" "$STAGE0_FALLBACK_TAG"
    RELEASE_JSON="$(fetch_release_json "$STAGE0_FALLBACK_TAG")" || RELEASE_JSON=""
    if [ -n "$RELEASE_JSON" ]; then
        DOWNLOAD_URL="$(find_asset_url "$RELEASE_JSON")"
    fi
fi

if [ -z "$DOWNLOAD_URL" ]; then
    printf 'Advarsel: Ingen pre-bygd binary for %s i GitHub Releases.\n' "$PLATFORM" >&2
    printf 'Feil: Kunne ikke installere Norscode for %s.\n' "$PLATFORM" >&2
    printf 'Alternativ: klon repoet og kjør: bash tools/install.sh\n' >&2
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

if [ "$OS" = "Darwin" ] && [ -x "$ROOT_DIR/tools/install-macos-file-icons.sh" ]; then
    bash "$ROOT_DIR/tools/install-macos-file-icons.sh" || true
fi

printf '\nNorscode installert!\n'
printf '  Binary: %s/norscode\n' "$BIN_DIR"
printf '  Alias:  %s/nc\n' "$BIN_DIR"
printf '\nLegg til PATH (bash/zsh):\n'
printf '  export PATH="%s:$PATH"\n' "$BIN_DIR"
printf '\nTest:\n'
printf '  norscode --help\n'
