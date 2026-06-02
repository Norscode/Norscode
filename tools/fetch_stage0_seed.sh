#!/usr/bin/env bash
# tools/fetch_stage0_seed.sh — last ned norscode-<plattform> til bootstrap/stage0/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

platform_name() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"
    case "$OS" in
        Darwin)
            case "$ARCH" in arm64) printf 'macos-arm64' ;; x86_64) printf 'macos-x86_64' ;; *) return 1 ;; esac ;;
        Linux)
            case "$ARCH" in x86_64|amd64) printf 'linux-x86_64' ;; aarch64|arm64) printf 'linux-arm64' ;; *) return 1 ;; esac ;;
        *) return 1 ;;
    esac
}

platform="$(platform_name)" || {
    printf 'Feil: ukjent plattform\n' >&2
    exit 1
}

DEST="${ROOT}/bootstrap/stage0/norscode-${platform}"
mkdir -p "$(dirname "$DEST")"

if [ -f "$DEST" ]; then
    printf '✓ stage0-seed finst: %s (%d bytes)\n' "$DEST" "$(wc -c < "$DEST" | tr -d ' ')"
    exit 0
fi

repo="${NORSCODE_RELEASE_REPO:-${GITHUB_REPOSITORY:-Norscode/Norscode}}"
asset_name="norscode-${platform}"
releases_url="https://api.github.com/repos/${repo}/releases/latest"
token="${GITHUB_TOKEN:-${GH_TOKEN:-}}"

if command -v curl >/dev/null 2>&1; then
    if [ -n "$token" ]; then
        fetch_json=(curl -fsSL -H "Authorization: Bearer $token" -H "Accept: application/vnd.github+json")
        fetch_file=(curl -fsSL -L -H "Authorization: Bearer $token" -H "Accept: application/octet-stream")
    else
        fetch_json=(curl -fsSL)
        fetch_file=(curl -fsSL -L)
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
    printf 'Feil: trenger curl eller wget\n' >&2
    exit 1
fi

printf 'Hentar %s frå %s ...\n' "$asset_name" "$repo"
release_json="$("${fetch_json[@]}" "$releases_url")" || {
    printf 'Feil: kunne ikkje hente release-info\n' >&2
    exit 1
}

if command -v jq >/dev/null 2>&1; then
    download_url="$(printf '%s' "$release_json" | jq -r --arg n "$asset_name" \
        '.assets[] | select(.name == $n or (.name | startswith($n))) | .browser_download_url' | head -1)"
else
    download_url="$(printf '%s' "$release_json" | grep -o "\"browser_download_url\":[[:space:]]*\"[^\"]*${asset_name}[^\"]*\"" | head -1 | cut -d'"' -f4)"
fi

if [ -z "$download_url" ] || [ "$download_url" = "null" ]; then
    printf 'Feil: ingen release-asset %s\n' "$asset_name" >&2
    printf 'Legg fil i bootstrap/stage0/ eller publiser ein release.\n' >&2
    exit 1
fi

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT
"${fetch_file[@]}" "$download_url" > "$tmp"
mv "$tmp" "$DEST"
chmod +x "$DEST"
trap - EXIT

# Røyk (same som build_norscode_native)
tmp_no="$(mktemp "${TMPDIR:-/tmp}/nc_seed_smoke_XXXXXX.no" 2>/dev/null || echo "${TMPDIR:-/tmp}/nc_seed_$$.no")"
printf 'funksjon start() { returner 0 }\n' > "$tmp_no"
if ! NORSCODE_CMD=run NORSCODE_FILE="$tmp_no" "$DEST" >/dev/null 2>&1; then
    rm -f "$tmp_no" "$DEST"
    printf 'Feil: nedlasta seed feila røyktest\n' >&2
    exit 1
fi
rm -f "$tmp_no"

printf '✓ stage0-seed: %s (%d bytes)\n' "$DEST" "$(wc -c < "$DEST" | tr -d ' ')"
