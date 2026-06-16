#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Sikrar at dist/norscode_native finst (bootstrap-/stage-0 runtime).
#
# Rekkefølge (default / normal flyt):
#   1. Eksisterande, fungerande dist/norscode_native
#   2. bootstrap/stage0/norscode-<plattform> (seed)
#   3. GitHub Release (seed)
#   4. NORSCODE_BOOTSTRAP_C=1 + isolert historisk output + clang (berre historisk vedlikehald / regen)
# Den gjenværende C-hostgrensa er bootstrap, ikkje normal kjede.
#
# Ved REGEN=1 (historisk vedlikehald): seed → tools/maint/regen_native.sh → valfri BOOTSTRAP_C_ROOT → clang
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norscode_native"
REGEN="${REGEN:-0}"
SMOKE_ENABLED="${NORSCODE_REQUIRE_SMOKE:-1}"
BOOTSTRAP_C_ROOT="${BOOTSTRAP_C_ROOT:-${ROOT}/build/maintainer_regen}"
STATIC_STAGE0="${NORSCODE_STATIC_STAGE0:-0}"

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
        printf '⚠️  Dist-smoke er slått av (NORSCODE_REQUIRE_SMOKE=0)\n'
        return 0
    fi
    if smoke_ok "$1"; then
        return 0
    fi
    if [ "${CI:-0}" != "1" ] && same_bytes_as_stage0_seed "$1"; then
        printf '⚠️  Dist-smoke feila lokalt, men binæren matcher committed stage-0-seed byte for byte.\n'
        printf '⚠️  Held fram lokalt; CI krev framleis full smoke.\n'
        return 0
    fi
    return 1
}

copy_stage0_binary() {
    platform="$(platform_name)" || return 1
    stage0="${ROOT}/bootstrap/stage0/norscode-${platform}"
    [ -f "$stage0" ] || return 1
    mkdir -p "$(dirname "$OUT")"
    rm -f "$OUT"
    cp "$stage0" "$OUT"
    chmod +x "$OUT"
    case "$(uname -s)" in
        Darwin)
            # Materialiser ein rein lokal kopi på macOS.
            # Direkte stage-0-symlink kan bli drepen av vertsmetadata/provenance sjølv når binæren er byte-lik.
            xattr -c "$OUT" 2>/dev/null || true
            ;;
    esac
    printf "✓ dist/norscode_native ← bootstrap/stage0/norscode-%s (%d bytes)\n" \
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
    return 0
}

bootstrap_c_present() {
    [ -f "${BOOTSTRAP_C_ROOT}/maint/c/norscode_generated.c" ]
}

regen_bootstrap_c() {
    if ! [ -x "${ROOT}/dist/norscode_native" ]; then
        printf 'Feil: trenger seed (dist/norscode_native) for regen\n' >&2
        return 1
    fi
    printf 'Historisk vedlikehaldsmodus: regenererer output frå .no i %s/maint/c/ (L6)...\n' "$BOOTSTRAP_C_ROOT" >&2
    REGEN_ROOT="${BOOTSTRAP_C_ROOT}" bash "${ROOT}/tools/maint/regen_native.sh" || return 1
    bootstrap_c_present
}

build_from_bootstrap_c() {
    local gen="${BOOTSTRAP_C_ROOT}/maint/c/norscode_generated.c"
    local main
    if [ -f "${ROOT}/tools/maint/c/nc_native_main.c" ]; then
        main="${ROOT}/tools/maint/c/nc_native_main.c"
    else
        main="${ROOT}/archive/legacy_c_backend/nc_native_main.c"
    fi
    local tmp=""

    for f in "$gen" "$main"; do
        if [ ! -f "$f" ]; then
            printf 'Feil: manglar %s (stage-0 C-kjelde)\n' "$f" >&2
            return 1
        fi
    done

    CC="${CC:-clang}"
    command -v "$CC" >/dev/null 2>&1 || CC=cc
    command -v "$CC" >/dev/null 2>&1 || {
        printf 'Feil: historisk regenerering krev clang eller gcc for stage-0-bygg\n' >&2
        return 1
    }

    mkdir -p "$(dirname "$OUT")"
    tmp="$(mktemp "${TMPDIR:-/tmp}/nc_native_XXXXXX.c" 2>/dev/null || echo "${TMPDIR:-/tmp}/nc_native_$$.c")"
    trap 'test -n "${tmp:-}" && rm -f "$tmp"' EXIT

    # norscode_generated.c har runtime innebygd; ikkje legg til tools/maint/ runtime-filer eksplisitt
    {
        printf '/* Host FFI forward decl (Omgang 4) */\n'
        printf 'struct NcVal;\n'
        printf 'typedef struct NcVal NcVal;\n'
        printf 'NcVal *nc_fn_builtin_host_exec_ncb_json(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_host_kall_bygg_bundle(NcVal **args, int na);\n\n'
        printf 'NcVal *nc_fn_builtin_ast_normaliser_type(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_analyser_program(NcVal **args, int na);\n\n'
        grep -v '#include.*nc_runtime' "$gen" | sed 's/^int main/static int nc_gen_main/'
    } >> "$tmp"
    cat "$main" >> "$tmp"

    printf 'Historisk vedlikehaldsmodus: kompilerer norscode_native frå %s/maint/c (stage-0, %s)...\n' "$BOOTSTRAP_C_ROOT" "$CC" >&2
    # Detect sqlite3 linkage
    # On macOS, sqlite3 ships with Xcode/CommandLineTools → always use -lsqlite3.
    # On Linux, prefer -lsqlite3 (libsqlite3-dev); fall back to direct .so path.
    _sqlite_flag="-lsqlite3"
    _static_flag=""
    _static_extra_libs=""
    case "$(uname -s)" in
        Linux)
            if [ "$STATIC_STAGE0" = "1" ]; then
                _static_flag="-static"
                _static_extra_libs="-lm"
            fi
            # Test with a trivial source to confirm -lsqlite3 links
            _sqlite_test=$(mktemp /tmp/nc_sq_XXXXXX.c)
            printf 'int sqlite3_open(const char*,void**); int main(){return 0;}\n' > "$_sqlite_test"
            if ! "$CC" -o /dev/null "$_sqlite_test" -lsqlite3 2>/dev/null; then
                _found=$(find /usr/lib /usr/local/lib 2>/dev/null -name "libsqlite3.so*" | sort | head -1)
                [ -n "$_found" ] && _sqlite_flag="-Wl,$_found" || _sqlite_flag=""
            fi
            rm -f "$_sqlite_test"
            ;;
    esac
    if ! "$CC" -O2 -Wno-everything -DNORSCODE_NATIVE_MAIN -o "$OUT" "$tmp" $_static_flag $_sqlite_flag $_static_extra_libs; then
        _clang_ec=$?
        rm -f "$tmp"
        tmp=""
        printf 'Feil: clang kompilering feila (exit %d)\n' "$_clang_ec" >&2
        return 1
    fi
    rm -f "$tmp"
    tmp=""
    trap - EXIT
    chmod +x "$OUT"

    smoke_ok "$OUT" || {
        printf 'Feil: bygget binær feila NORSCODE_CMD-røyktest\n' >&2
        return 1
    }
    printf "✓ dist/norscode_native bygget i historisk vedlikehaldsmodus frå %s/maint/c (%d bytes)\n" "$BOOTSTRAP_C_ROOT" "$(wc -c < "$OUT" | tr -d ' ')"
    return 0
}

if [ "$REGEN" != "1" ] && [ -x "$OUT" ] && smoke_check "$OUT"; then
    :
    printf "✓ dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

mkdir -p "$(dirname "$OUT")"

if copy_stage0_binary; then :
elif download_release_binary; then
    printf "✓ dist/norscode_native lasta ned frå release (%d bytes)\n" "$(wc -c < "$OUT" | tr -d ' ')"
else
    platform="$(platform_name 2>/dev/null || printf '?')"
    printf '\n=== Kunne ikkje skaffe norscode_native ===\n' >&2
    printf 'Køyr: bash tools/fetch_stage0_seed.sh\n' >&2
    printf 'Eller legg norscode-%s i bootstrap/stage0/,\n' "$platform" >&2
    printf 'eller last ned frå release i workflow via ensure_stage0_seed / fetch_stage0_seed.\n' >&2
    exit 1
fi

if [ "$REGEN" = "1" ]; then
    if [ "${NORSCODE_BOOTSTRAP_C:-0}" != "1" ]; then
        printf 'Feil: REGEN=1 er historisk vedlikehaldsmodus og krev NORSCODE_BOOTSTRAP_C=1\n' >&2
        exit 1
    fi

    if bootstrap_c_present && build_from_bootstrap_c; then exit 0; fi
    if regen_bootstrap_c && build_from_bootstrap_c; then exit 0; fi
    platform="$(platform_name 2>/dev/null || printf '?')"
    printf '\n=== Kunne ikkje byggje norscode_native etter regen ===\n' >&2
    printf 'Sjekk historiske vedlikehaldsføresetnader og køyr: bash tools/maint/regen_native.sh --rebuild\n' >&2
    exit 1
fi

if smoke_check "$OUT"; then
    printf 'ℹ︎ Klar (ingen regen). Set berre REGEN=1 i eksplisitt historisk vedlikehaldsmodus for stage-0 regen + clang.\n'
    exit 0
fi

printf 'Feil: norscode_native feila røyktest\n' >&2
exit 1
