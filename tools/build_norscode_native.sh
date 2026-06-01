#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Sikrar at dist/norscode_native finst (NORSCODE_CMD-runtime).
#
# Rekkefølge:
#   1. Eksisterande, fungerande dist/norscode_native
#   2. bootstrap/stage0/norscode-<plattform>
#   3. GitHub Release
#   4. Kompiler frå bootstrap/c/*.c + tools/nc_*.c (clang, stage-0 — ingen Python)
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

smoke_ok() {
    [ -x "$1" ] || return 1
    local tmp
    tmp="$(mktemp /tmp/nc_smoke_XXXXXX.no)"
    printf 'funksjon start() { returner 42 }\n' > "$tmp"
    if NORSCODE_CMD=run NORSCODE_FILE="$tmp" "$1" >/dev/null 2>&1; then
        rm -f "$tmp"
        return 0
    fi
    rm -f "$tmp"
    return 1
}

copy_stage0_binary() {
    platform="$(platform_name)" || return 1
    stage0="${ROOT}/bootstrap/stage0/norscode-${platform}"
    [ -f "$stage0" ] || return 1
    mkdir -p "$(dirname "$OUT")"
    cp "$stage0" "$OUT"
    chmod +x "$OUT"
    smoke_ok "$OUT" || return 1
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

    release_json="$("${fetch_json[@]}" "$releases_url" 2>/dev/null)" || return 1

    if command -v jq >/dev/null 2>&1; then
        download_url="$(printf '%s' "$release_json" | jq -r --arg n "$asset_name" \
            '.assets[] | select(.name == $n or (.name | startswith($n))) | .browser_download_url' | head -1)"
    else
        download_url="$(printf '%s' "$release_json" | grep -o "\"browser_download_url\":[[:space:]]*\"[^\"]*${asset_name}[^\"]*\"" | head -1 | cut -d'"' -f4)"
    fi

    [ -n "$download_url" ] && [ "$download_url" != "null" ] || return 1

    tmp_file="$(mktemp)"
    trap 'rm -f "$tmp_file"' EXIT
    "${fetch_file[@]}" "$download_url" > "$tmp_file" || { rm -f "$tmp_file"; trap - EXIT; return 1; }
    mkdir -p "$(dirname "$OUT")"
    mv "$tmp_file" "$OUT"
    chmod +x "$OUT"
    trap - EXIT
    smoke_ok "$OUT" || return 1
    return 0
}

build_from_bootstrap_c() {
    local gen="${ROOT}/bootstrap/c/norscode_generated.c"
    local disp="${ROOT}/bootstrap/c/nc_dispatch.c"
    local main="${ROOT}/tools/nc_native_main.c"

    for f in "$gen" "$disp" "$main"; do
        if [ ! -f "$f" ]; then
            printf 'Feil: manglar %s (stage-0 C-kjelde)\n' "$f" >&2
            return 1
        fi
    done

    CC="${CC:-clang}"
    command -v "$CC" >/dev/null 2>&1 || CC=cc
    command -v "$CC" >/dev/null 2>&1 || {
        printf 'Feil: trenger clang eller gcc for stage-0-bygg\n' >&2
        return 1
    }

    mkdir -p "$(dirname "$OUT")"
    local tmp
    tmp="$(mktemp /tmp/nc_native_XXXXXX.c)"
    trap 'rm -f "$tmp"' EXIT

    # norscode_generated.c har runtime innebygd; ikkje legg til nc_runtime_mini.c
    grep -v '#include.*nc_runtime' "$gen" | sed 's/^int main/static int nc_gen_main/' >> "$tmp"
    cat "$disp" >> "$tmp"
    cat "$main" >> "$tmp"

    printf 'Kompilerer norscode_native frå bootstrap/c (stage-0, %s)...\n' "$CC" >&2
    "$CC" -O2 -Wno-everything -o "$OUT" "$tmp"
    rm -f "$tmp"
    trap - EXIT
    chmod +x "$OUT"

    smoke_ok "$OUT" || {
        printf 'Feil: bygget binær feila NORSCODE_CMD-røyktest\n' >&2
        return 1
    }
    printf "✓ dist/norscode_native bygget frå bootstrap/c (%d bytes)\n" "$(wc -c < "$OUT" | tr -d ' ')"
    return 0
}

if [ -x "$OUT" ] && smoke_ok "$OUT"; then
    printf "✓ dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

mkdir -p "$(dirname "$OUT")"

if copy_stage0_binary; then exit 0; fi

if download_release_binary; then
    printf "✓ dist/norscode_native lasta ned frå release (%d bytes)\n" "$(wc -c < "$OUT" | tr -d ' ')"
    exit 0
fi

if build_from_bootstrap_c; then exit 0; fi

platform="$(platform_name 2>/dev/null || printf '?')"
printf '\n=== Kunne ikkje skaffe norscode_native ===\n' >&2
printf 'Prøv: bash tools/build_norscode_native.sh (krev bootstrap/c + clang)\n' >&2
printf 'Eller legg norscode-%s i bootstrap/stage0/\n' "$platform" >&2
exit 1
