#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Sikrar at dist/norscode_native finst for den aktive plattforma.
#
# Rekkefølge:
#   1. Eksisterande dist/norscode_native
#   2. bootstrap/stage0/norscode-<plattform>
#   3. GitHub Release (krev GITHUB_TOKEN på privat repo)
#
# CI: GITHUB_TOKEN og GITHUB_REPOSITORY er sett i workflow-env.
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
                aarch64|arm64) printf 'linux-arm64' ;;
                *) return 1 ;;
            esac
            ;;
        *)
            return 1
            ;;
    esac
}

copy_stage0_binary() {
    platform="$(platform_name)" || return 1
    stage0="${ROOT}/bootstrap/stage0/norscode-${platform}"
    if [ ! -f "$stage0" ]; then
        return 1
    fi
    mkdir -p "$(dirname "$OUT")"
    cp "$stage0" "$OUT"
    chmod +x "$OUT"
    printf "✓ dist/norscode_native frå bootstrap/stage0/norscode-%s (%d bytes)\n" \
        "$platform" "$(wc -c < "$OUT" | tr -d ' ')"
    return 0
}

download_release_binary() {
    repo="${NORSCODE_RELEASE_REPO:-${GITHUB_REPOSITORY:-Norscode/Norscode}}"
    platform="$(platform_name)" || return 1
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
        return 1
    fi

    release_json="$("${fetch_json[@]}" "$releases_url" 2>/dev/null)" || {
        if [ "${GITHUB_ACTIONS:-}" = "true" ]; then
            printf 'CI: klarte ikkje hente %s (sjekk token og at release finst)\n' "$releases_url" >&2
        fi
        return 1
    }

    if command -v jq >/dev/null 2>&1; then
        download_url="$(printf '%s' "$release_json" | jq -r --arg n "$asset_name" \
            '.assets[] | select(.name == $n or (.name | startswith($n))) | .browser_download_url' | head -1)"
    else
        download_url="$(printf '%s' "$release_json" | grep -o "\"browser_download_url\":[[:space:]]*\"[^\"]*${asset_name}[^\"]*\"" | head -1 | cut -d'"' -f4)"
    fi

    if [ -z "$download_url" ] || [ "$download_url" = "null" ]; then
        if [ "${GITHUB_ACTIONS:-}" = "true" ] && command -v jq >/dev/null 2>&1; then
            printf 'CI: release har ikkje asset %s. Tilgjengelege:\n' "$asset_name" >&2
            printf '%s' "$release_json" | jq -r '.assets[].name' >&2 || true
        fi
        return 1
    fi

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

ci_fail_help() {
    platform="$(platform_name 2>/dev/null || printf '?')"
    printf '\n' >&2
    printf '=== Stage-0 manglar (norscode_native) ===\n' >&2
    printf 'CI treng ein ferdig binær som støttar NORSCODE_CMD=run (ikkje berre C-bootstrap).\n' >&2
    printf '\n' >&2
    printf 'Løysing (vel éin):\n' >&2
    printf '  1. Publiser GitHub Release med asset: norscode-%s\n' "$platform" >&2
    printf '  2. Legg same fil i bootstrap/stage0/norscode-%s og commit\n' "$platform" >&2
    printf '  3. Sjå bootstrap/stage0/README.md\n' >&2
    printf '\n' >&2
}

if [ -x "$OUT" ]; then
    printf "✓ dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

mkdir -p "$(dirname "$OUT")"

if copy_stage0_binary; then
    exit 0
fi

if download_release_binary; then
    SIZE="$(wc -c < "$OUT")"
    printf "✓ dist/norscode_native lasta ned frå release (%d bytes)\n" "$SIZE"
    exit 0
fi

# C-bootstrap kan selfcheck, men dekkar ikkje ./bin/nc test — bygg berre for diagnose
if [ "${GITHUB_ACTIONS:-}" = "true" ]; then
    if bash "$ROOT/tools/build_norscode_native_from_source.sh"; then
        printf 'CI: norcode-bootstrap-compile bygget, men norscode_native manglar framleis.\n' >&2
    fi
fi

ci_fail_help
exit 1
