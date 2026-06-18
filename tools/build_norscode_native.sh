#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Sikrar at dist/norscode_native finst utan C eller Python.
#
# Rekkefølge:
#   1. Eksisterande, fungerande dist/norscode_native
#   2. bootstrap/stage0/norscode-<plattform> (committed seed)
#   3. GitHub Release (ferdig native seed)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norscode_native"
SMOKE_ENABLED="${NORSCODE_REQUIRE_SMOKE:-1}"

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

stage0_seed_path() {
    local platform
    platform="$(platform_name)" || return 1
    printf '%s/bootstrap/stage0/norscode-%s' "$ROOT" "$platform"
}

same_bytes_as_stage0_seed() {
    local candidate="$1"
    local seed
    local cand_sha
    local seed_sha
    seed="$(stage0_seed_path)" || return 1
    [ -f "$seed" ] || return 1
    if command -v shasum >/dev/null 2>&1; then
        cand_sha="$(shasum -a 256 "$candidate" | awk '{print $1}')" || return 1
        seed_sha="$(shasum -a 256 "$seed" | awk '{print $1}')" || return 1
        [ "$cand_sha" = "$seed_sha" ]
        return $?
    fi
    if command -v sha256sum >/dev/null 2>&1; then
        cand_sha="$(sha256sum "$candidate" | awk '{print $1}')" || return 1
        seed_sha="$(sha256sum "$seed" | awk '{print $1}')" || return 1
        [ "$cand_sha" = "$seed_sha" ]
        return $?
    fi
    return 1
}

smoke_ok() {
    [ -x "$1" ] || return 1
    local tmp
    tmp="$(mktemp "${TMPDIR:-/tmp}/nc_smoke_XXXXXX.no" 2>/dev/null || true)"
    if [ -z "$tmp" ]; then
        tmp="${TMPDIR:-/tmp}/nc_smoke_$$.no"
    fi
    rm -f "$tmp"
    printf 'funksjon start() { returner 0 }\n' > "$tmp"
    if NORSCODE_CMD=run NORSCODE_FILE="$tmp" "$1" >/dev/null 2>&1; then
        rm -f "$tmp"
        return 0
    fi
    rm -f "$tmp"
    return 1
}

smoke_check() {
    if [ "$SMOKE_ENABLED" != "1" ]; then
        printf 'Dist-smoke er slått av (NORSCODE_REQUIRE_SMOKE=0)\n'
        return 0
    fi
    if smoke_ok "$1"; then
        return 0
    fi
    if [ "${CI:-0}" != "1" ] && same_bytes_as_stage0_seed "$1"; then
        printf 'Dist-smoke feila lokalt, men binæren matcher committed stage-0-seed byte for byte.\n'
        printf 'Held fram lokalt; CI krev framleis full smoke.\n'
        return 0
    fi
    return 1
}

copy_stage0_binary() {
    local platform
    local stage0
    platform="$(platform_name)" || return 1
    stage0="${ROOT}/bootstrap/stage0/norscode-${platform}"
    [ -f "$stage0" ] || return 1
    mkdir -p "$(dirname "$OUT")"
    rm -f "$OUT"
    cp "$stage0" "$OUT"
    chmod +x "$OUT"
    case "$(uname -s)" in
        Darwin)
            xattr -c "$OUT" 2>/dev/null || true
            ;;
    esac
    printf "dist/norscode_native <- bootstrap/stage0/norscode-%s (%d bytes)\n" \
        "$platform" "$(wc -c < "$OUT" | tr -d ' ')"
    return 0
}

download_release_binary() {
    local repo
    local platform
    local asset_name
    local releases_url
    local token
    local release_json
    local download_url
    local tmp_file

    repo="${NORSCODE_RELEASE_REPO:-${GITHUB_REPOSITORY:-Norscode/Norscode}}"
    platform="$(platform_name)" || return 1
    asset_name="norscode-${platform}"
    releases_url="https://api.github.com/repos/${repo}/releases/latest"
    token="${GITHUB_TOKEN:-${GH_TOKEN:-}}"

    if command -v curl >/dev/null 2>&1; then
        if [ -n "$token" ]; then
            release_json="$(curl -fsSL -H "Authorization: Bearer $token" -H "Accept: application/vnd.github+json" "$releases_url" 2>/dev/null)" || return 1
        else
            release_json="$(curl -fsSL "$releases_url" 2>/dev/null)" || return 1
        fi
    elif command -v wget >/dev/null 2>&1; then
        if [ -n "$token" ]; then
            release_json="$(wget -qO- --header="Authorization: Bearer $token" --header="Accept: application/vnd.github+json" "$releases_url" 2>/dev/null)" || return 1
        else
            release_json="$(wget -qO- "$releases_url" 2>/dev/null)" || return 1
        fi
    else
        return 1
    fi

    if command -v jq >/dev/null 2>&1; then
        download_url="$(printf '%s' "$release_json" | jq -r --arg n "$asset_name" \
            '.assets[] | select(.name == $n or (.name | startswith($n))) | .browser_download_url' | head -1)"
    else
        download_url="$(printf '%s' "$release_json" | grep -o "\"browser_download_url\":[[:space:]]*\"[^\"]*${asset_name}[^\"]*\"" | head -1 | cut -d'"' -f4)"
    fi

    [ -n "$download_url" ] && [ "$download_url" != "null" ] || return 1

    tmp_file="$(mktemp)"
    trap 'rm -f "$tmp_file"' EXIT
    if command -v curl >/dev/null 2>&1; then
        if [ -n "$token" ]; then
            curl -fsSL -L -H "Authorization: Bearer $token" -H "Accept: application/octet-stream" "$download_url" > "$tmp_file" || return 1
        else
            curl -fsSL -L "$download_url" > "$tmp_file" || return 1
        fi
    else
        if [ -n "$token" ]; then
            wget -qO- --header="Authorization: Bearer $token" --header="Accept: application/octet-stream" "$download_url" > "$tmp_file" || return 1
        else
            wget -qO- "$download_url" > "$tmp_file" || return 1
        fi
    fi
    mkdir -p "$(dirname "$OUT")"
    mv "$tmp_file" "$OUT"
    chmod +x "$OUT"
    trap - EXIT
    return 0
}

if [ -x "$OUT" ] && smoke_check "$OUT"; then
    printf "dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

mkdir -p "$(dirname "$OUT")"

if copy_stage0_binary; then
    :
elif download_release_binary; then
    printf "dist/norscode_native lasta ned frå release (%d bytes)\n" "$(wc -c < "$OUT" | tr -d ' ')"
else
    platform="$(platform_name 2>/dev/null || printf '?')"
    printf '\n=== Kunne ikkje skaffe norscode_native utan C/Python ===\n' >&2
    printf 'Legg norscode-%s i bootstrap/stage0/ eller publiser ein release med ferdig native seed.\n' "$platform" >&2
    exit 1
fi

if smoke_check "$OUT"; then
    printf 'Klar: native runtime er materialisert utan C/Python.\n'
    exit 0
fi

printf 'Feil: norscode_native feila røyktest\n' >&2
exit 1
