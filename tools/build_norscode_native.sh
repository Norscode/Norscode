#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Sikrar at dist/norscode_native finst for den aktive plattforma.
#
# Rekkefølge:
#   1. Eksisterande dist/norscode_native
#   2. bootstrap/stage0/norscode-<plattform>
#   3. GitHub Release (GITHUB_TOKEN på privat repo)
#   4. Linux CI: Docker (Dockerfile.linux-build) — bootstrap-binær
#
# Merk: Steg 4 gir berre C/NCBB-bootstrap; ./bin/nc test treng NORSCODE_CMD-runtime.
#       Legg ekte binær i bootstrap/stage0/ eller publiser release (sjå README der).
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

ensure_embed_c_files() {
    local need=0
    for f in \
        build/bootstrap_compiler_bundle_ncb_data.c \
        build/native_elf_compiler_bundle_ncb_data.c
    do
        if [ ! -f "$ROOT/$f" ]; then
            need=1
        fi
    done
    if [ "$need" -eq 1 ]; then
        bash "$ROOT/tools/generate_build_embed_c.sh"
    fi
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
            printf 'CI: klarte ikkje hente %s (ingen release eller manglar tilgang)\n' "$releases_url" >&2
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
    mkdir -p "$(dirname "$OUT")"
    mv "$tmp_file" "$OUT"
    chmod +x "$OUT"
    trap - EXIT
    return 0
}

build_linux_via_docker() {
    [ "$(uname -s)" = "Linux" ] || return 1
    command -v docker >/dev/null 2>&1 || return 1
    ensure_embed_c_files
    if [ ! -f "$ROOT/build/mv_bootstrap_stub_manifest_dispatch.inc" ]; then
        printf 'Feil: manglar build/mv_bootstrap_stub_manifest_dispatch.inc\n' >&2
        return 1
    fi
    local dest="${ROOT}/.ci-docker-native-out"
    rm -rf "$dest"
    mkdir -p "$dest"
    printf 'CI: byggjer Linux bootstrap via Docker...\n' >&2
    if ! docker buildx build --platform linux/amd64 -f "$ROOT/Dockerfile.linux-build" \
        --output "type=local,dest=$dest" "$ROOT" >&2; then
        return 1
    fi
    if [ ! -f "$dest/norscode-linux-x86_64" ]; then
        return 1
    fi
    mkdir -p "$(dirname "$OUT")"
    cp "$dest/norscode-linux-x86_64" "$OUT"
    chmod +x "$OUT"
    printf '✓ dist/norscode_native frå Docker (%d bytes) — kan mangle NORSCODE_CMD; legg inn stage0/release\n' \
        "$(wc -c < "$OUT" | tr -d ' ')" >&2
    return 0
}

ci_fail_help() {
    platform="$(platform_name 2>/dev/null || printf '?')"
    printf '\n=== Stage-0 manglar (norscode_native) ===\n' >&2
    printf 'CI treng binær med NORSCODE_CMD=run for ./bin/nc test.\n' >&2
    printf '\nLøysing:\n' >&2
    printf '  1. Legg %s i bootstrap/stage0/norscode-%s og commit\n' "fil" "$platform" >&2
    printf '  2. Publiser GitHub Release med asset norscode-%s\n' "$platform" >&2
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
    printf "✓ dist/norscode_native lasta ned frå release (%d bytes)\n" "$(wc -c < "$OUT" | tr -d ' ')"
    exit 0
fi

if [ "${GITHUB_ACTIONS:-}" = "true" ] && build_linux_via_docker; then
    exit 0
fi

ci_fail_help
exit 1
