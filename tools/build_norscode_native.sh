#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Sikrar at dist/norscode_native finst (bootstrap-/stage-0 runtime).
#
# Rekkefølge (default):
#   1. Eksisterande, fungerande dist/norscode_native
#   2. bootstrap/stage0/norscode-<plattform> (seed)
#   3. GitHub Release (seed)
#   4. NORSCODE_BOOTSTRAP_C=1 + bootstrap/maint/c/ + clang (berre maintainer / regen)
# Den gjenværende C-hostgrensa er bootstrap, ikkje normal kjede.
#
# Ved REGEN=1 (maintainer): seed → tools/maint/regen_native.sh → bootstrap/maint/c/ → clang
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norscode_native"
REGEN="${REGEN:-0}"

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

bootstrap_c_present() {
    [ -f "${ROOT}/bootstrap/maint/c/norscode_generated.c" ] \
        && [ -f "${ROOT}/bootstrap/maint/c/nc_dispatch.c" ]
}

regen_bootstrap_c() {
    if ! [ -x "${ROOT}/dist/norscode_native" ]; then
        printf 'Feil: trenger seed (dist/norscode_native) for regen\n' >&2
        return 1
    fi
    printf 'Regenererer bootstrap/maint/c/ frå .no (L6)...\n' >&2
    REGEN_ROOT="${ROOT}/bootstrap" bash "${ROOT}/tools/maint/regen_native.sh" || return 1
    bootstrap_c_present
}

build_from_bootstrap_c() {
    local gen="${ROOT}/bootstrap/maint/c/norscode_generated.c"
    local disp="${ROOT}/bootstrap/maint/c/nc_dispatch.c"
    local main="${ROOT}/tools/maint/c/nc_native_main.c"

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
    tmp="$(mktemp "${TMPDIR:-/tmp}/nc_native_XXXXXX.c" 2>/dev/null || echo "${TMPDIR:-/tmp}/nc_native_$$.c")"
    trap 'rm -f "$tmp"' EXIT

    # norscode_generated.c har runtime innebygd; ikkje legg til tools/maint/c/nc_runtime_mini.c
    {
        printf '/* Host FFI forward decl (Omgang 4) */\n'
        printf 'struct NcVal;\n'
        printf 'typedef struct NcVal NcVal;\n'
        printf 'NcVal *nc_fn_builtin_host_exec_ncb_json(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_host_kall_bygg_bundle(NcVal **args, int na);\n\n'
        grep -v '#include.*nc_runtime' "$gen" | sed 's/^int main/static int nc_gen_main/'
    } >> "$tmp"
    cat "$disp" >> "$tmp"
    cat "$main" >> "$tmp"

    printf 'Kompilerer norscode_native frå bootstrap/maint/c (stage-0, %s)...\n' "$CC" >&2
    # Detect sqlite3 linkage: prefer -lsqlite3 (needs dev headers), fall back to versioned .so
    _sqlite_flag="-lsqlite3"
    if ! "$CC" -lsqlite3 2>/dev/null; then
        _found=$(find /usr/lib /usr/local/lib 2>/dev/null -name "libsqlite3.so*" | sort | head -1)
        [ -n "$_found" ] && _sqlite_flag="-Wl,$_found" || _sqlite_flag=""
    fi 2>/dev/null
    if ! "$CC" -O2 -Wno-everything -o "$OUT" "$tmp" $_sqlite_flag; then
        _clang_ec=$?
        rm -f "$tmp"
        printf 'Feil: clang kompilering feila (exit %d)\n' "$_clang_ec" >&2
        return 1
    fi
    rm -f "$tmp"
    trap - EXIT
    chmod +x "$OUT"

    smoke_ok "$OUT" || {
        printf 'Feil: bygget binær feila NORSCODE_CMD-røyktest\n' >&2
        return 1
    }
    printf "✓ dist/norscode_native bygget frå bootstrap/maint/c (%d bytes)\n" "$(wc -c < "$OUT" | tr -d ' ')"
    return 0
}

if [ "$REGEN" != "1" ] && [ -x "$OUT" ] && smoke_ok "$OUT"; then
    printf "✓ dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

mkdir -p "$(dirname "$OUT")"

if copy_stage0_binary; then :
elif download_release_binary; then
    printf "✓ dist/norscode_native lasta ned frå release (%d bytes)\n" "$(wc -c < "$OUT" | tr -d ' ')"
elif [ "${NORSCODE_BOOTSTRAP_C:-0}" = "1" ] && bootstrap_c_present && build_from_bootstrap_c; then
    printf 'ℹ︎ Bygde frå bootstrap/maint/c/ (NORSCODE_BOOTSTRAP_C=1). Normal: bootstrap/stage0/ eller release.\n' >&2
else
    platform="$(platform_name 2>/dev/null || printf '?')"
    printf '\n=== Kunne ikkje skaffe norscode_native ===\n' >&2
    printf 'Køyr: bash tools/fetch_stage0_seed.sh\n' >&2
    printf 'Eller legg norscode-%s i bootstrap/stage0/, eller sett NORSCODE_BOOTSTRAP_C=1 med regenert bootstrap/maint/c/.\n' "$platform" >&2
    exit 1
fi

if [ "$REGEN" = "1" ]; then
    if regen_bootstrap_c && build_from_bootstrap_c; then exit 0; fi
    platform="$(platform_name 2>/dev/null || printf '?')"
    printf '\n=== Kunne ikkje byggje norscode_native etter regen ===\n' >&2
    printf 'Sjekk clang og køyr: bash tools/maint/regen_native.sh --rebuild\n' >&2
    exit 1
fi

if smoke_ok "$OUT"; then
    printf 'ℹ︎ Klar (ingen regen). Set REGEN=1 for stage-0 regen + clang.\n'
    exit 0
fi

printf 'Feil: norscode_native feila røyktest\n' >&2
exit 1
