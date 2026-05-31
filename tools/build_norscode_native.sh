#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Sikrar at dist/norscode_native finst for den aktive plattforma.
#
# Normal veg:
#   - bruk lokal prebygget binary dersom den alt ligg i dist/
#   - elles henta siste release-asset frå GitHub og legg ho i dist/
#
# Dette gjer CI grønn på fresh checkout utan å krevje at binæren er sjekka inn.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norscode_native"

platform_name() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"
    case "$OS" in
        Darwin)
            case "$ARCH" in
                arm64) printf 'macos-arm64' ;;
                x86_64) printf 'macos-x86_64' ;;
                *) return 1 ;;
            esac
            ;;
        Linux)
            case "$ARCH" in
                x86_64) printf 'linux-x86_64' ;;
                aarch64) printf 'linux-arm64' ;;
                *) return 1 ;;
            esac
            ;;
        *)
            return 1
            ;;
    esac
}

download_release_binary() {
    repo="${GITHUB_REPOSITORY:-Norscode/Norscode}"
    platform="$(platform_name)" || return 1
    asset_name="norscode-${platform}"
    releases_url="https://api.github.com/repos/${repo}/releases/latest"
    token="${GITHUB_TOKEN:-${GH_TOKEN:-}}"

    if command -v curl >/dev/null 2>&1; then
        if [ -n "$token" ]; then
            fetch_json=(curl -fsSL -H "Authorization: Bearer $token" -H "Accept: application/vnd.github+json")
            fetch_file=(curl -fsSL -H "Authorization: Bearer $token" -H "Accept: application/octet-stream")
        else
            fetch_json=(curl -fsSL)
            fetch_file=(curl -fsSL)
        fi
    elif command -v wget >/dev/null 2>&1; then
        if [ -n "$token" ]; then
            fetch_json=(wget -qO- --header="Authorization: Bearer $token" --header="Accept: application/vnd.github+json")
            fetch_file=(wget -qO- --header="Authorization: Bearer $token" --header="Accept: application/octet-stream")
        else
            fetch_json=(wget -qO-)
            fetch_file=(wget -qO-)
        fi
    else
        return 1
    fi

    release_json="$("${fetch_json[@]}" "$releases_url" 2>/dev/null)" || return 1
    download_url="$(printf '%s' "$release_json" | grep -o "\"browser_download_url\": \"[^\"]*${asset_name}[^\"]*\"" | head -1 | cut -d'"' -f4)"
    [ -n "$download_url" ] || return 1

    tmp_file="$(mktemp)"
    trap 'rm -f "$tmp_file"' EXIT
    if ! "${fetch_file[@]}" "$download_url" > "$tmp_file"; then
        rm -f "$tmp_file"
        trap - EXIT
        return 1
    fi
    mv "$tmp_file" "$OUT"
    chmod +x "$OUT"
    trap - EXIT
    return 0
}

# Dersom binæren allereie eksisterer, er det ingenting å gjere
if [ -x "$OUT" ]; then
    printf "✓ dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

mkdir -p "$(dirname "$OUT")"

if download_release_binary; then
    SIZE="$(wc -c < "$OUT")"
    printf "✓ dist/norscode_native lasta ned frå release (%d bytes)\n" "$SIZE"
    exit 0
fi

printf "Feil: dist/norscode_native finst ikkje, og release-nedlasting mislukkast.\n" >&2
printf "Prøv å leggje inn ein release-asset for den aktive plattforma, eller bygg manuelt.\n" >&2
exit 1
