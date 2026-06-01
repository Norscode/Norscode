#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Sørg for at dist/norscode_native finst for denne plattforma.
# Først brukar vi lokal binær om han allereie ligg der.
# Viss han manglar, prøver vi å hente siste release-asset frå GitHub.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT_DIR}/dist/norscode_native"
REPO="${NORSCODE_RELEASE_REPO:-Norscode/Norscode}"

if [ -x "$OUT" ]; then
    printf "✓ dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

OS="$(uname -s)"
ARCH="$(uname -m)"
case "$OS" in
    Darwin)
        case "$ARCH" in
            arm64) PLATFORM="macos-arm64" ;;
            x86_64) PLATFORM="macos-x86_64" ;;
            *)
                printf "Feil: ukjent macOS-arkitektur: %s\n" "$ARCH" >&2
                exit 1
                ;;
        esac
        ;;
    Linux)
        case "$ARCH" in
            x86_64) PLATFORM="linux-x86_64" ;;
            aarch64|arm64) PLATFORM="linux-arm64" ;;
            *)
                printf "Feil: ukjent Linux-arkitektur: %s\n" "$ARCH" >&2
                exit 1
                ;;
        esac
        ;;
    *)
        printf "Feil: støttar ikkje plattforma %s/%s\n" "$OS" "$ARCH" >&2
        exit 1
        ;;
esac

RELEASES_URL="https://api.github.com/repos/${REPO}/releases/latest"
BINARY_NAME="norscode-${PLATFORM}"

if command -v curl >/dev/null 2>&1; then
    FETCH="curl -fsSL"
elif command -v wget >/dev/null 2>&1; then
    FETCH="wget -qO-"
else
    printf "Feil: treng curl eller wget for å hente release-asset.\n" >&2
    exit 1
fi

printf "dist/norscode_native manglar, prøver å hente %s frå siste release...\n" "$BINARY_NAME"

if [ -n "${GITHUB_TOKEN:-}" ]; then
    RELEASE_JSON="$(curl -fsSL -H "Authorization: Bearer ${GITHUB_TOKEN}" -H "Accept: application/vnd.github+json" "$RELEASES_URL")" || {
        printf "Feil: klarte ikkje å hente release-info frå GitHub.\n" >&2
        exit 1
    }
else
    RELEASE_JSON="$($FETCH "$RELEASES_URL")" || {
        printf "Feil: klarte ikkje å hente release-info frå GitHub.\n" >&2
        exit 1
    }
fi

DOWNLOAD_URL="$(printf '%s' "$RELEASE_JSON" | grep -o "\"browser_download_url\":[[:space:]]*\"[^\"]*${BINARY_NAME}[^\"]*\"" | head -1 | cut -d'"' -f4)"

if [ -z "$DOWNLOAD_URL" ]; then
    printf "Feil: release-asset for %s finst ikkje i siste release.\n" "$PLATFORM" >&2
    printf "Prøv å leggje inn ein release-asset for den aktive plattforma, eller bygg manuelt.\n" >&2
    exit 1
fi

mkdir -p "${ROOT_DIR}/dist"
TMP_OUT="$(mktemp "${OUT}.XXXXXX")"
trap 'rm -f "$TMP_OUT"' EXIT

if command -v curl >/dev/null 2>&1; then
    if [ -n "${GITHUB_TOKEN:-}" ]; then
        curl -fL -H "Authorization: Bearer ${GITHUB_TOKEN}" -H "Accept: application/octet-stream" "$DOWNLOAD_URL" -o "$TMP_OUT"
    else
        curl -fL "$DOWNLOAD_URL" -o "$TMP_OUT"
    fi
else
    wget -qO "$TMP_OUT" "$DOWNLOAD_URL"
fi

chmod +x "$TMP_OUT"
mv "$TMP_OUT" "$OUT"
trap - EXIT

printf "✓ dist/norscode_native lasta ned (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
