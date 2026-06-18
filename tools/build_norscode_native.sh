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
    local out_tmp
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
        printf 'NcVal *nc_fn_builtin_ncb_route_handlers(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_ncb_call_fn(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_tekst_til_liten(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_tekst_til_heltall(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_semantic_analyser(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_semantic_analyser_rent(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_semantic_ok(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_semantic_rapport(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_semantic_validate_program(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_semantic_has_errors(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_wasm_selftest(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_web_request_query_param(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_web_request_header(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_web_request_json(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_web_request_json_field(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_web_response_builder(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_web_response_error(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_web_has_role(NcVal **args, int na);\n'
        printf 'NcVal *nc_builtin_socket_new(NcVal *domain, NcVal *type);\n'
        printf 'NcVal *nc_builtin_socket_connect(NcVal *sock, NcVal *host, NcVal *port);\n'
        printf 'NcVal *nc_builtin_socket_bind(NcVal *sock, NcVal *host, NcVal *port);\n'
        printf 'NcVal *nc_builtin_socket_send(NcVal *sock, NcVal *data);\n'
        printf 'NcVal *nc_builtin_socket_send_bytes(NcVal *sock, NcVal *data);\n'
        printf 'NcVal *nc_builtin_socket_recv(NcVal *sock, NcVal *max);\n'
        printf 'NcVal *nc_builtin_socket_recv_bytes(NcVal *sock, NcVal *max);\n'
        printf 'NcVal *nc_builtin_socket_settimeout(NcVal *sock, NcVal *timeout);\n'
        printf 'NcVal *nc_fn_std_socket_ny_tcp(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_std_socket_lukk(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_std_socket_aksepter(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_std_socket_motta(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_std_socket_send(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_std_socket_er_feil(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_std_socket_bind(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_std_socket_lytt(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_std_socket_fil_deskriptor(NcVal **args, int na);\n'
        if ! grep -q '^static NcVal \*nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_til_ncb_json(NcVal \*\*args, int nargs) {' "$gen"; then
            printf 'NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_til_ncb_json(NcVal **args, int na);\n'
        fi
        printf '\n'
        grep -v '#include.*nc_runtime' "$gen" | sed 's/^int main/static int nc_gen_main/'
    } >> "$tmp"
    cat "$main" >> "$tmp"
    append_historical_host_shims "$tmp" "$gen"

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
    out_tmp="${OUT}.build.$$"
    rm -f "$out_tmp"
    if ! "$CC" -O2 -Wno-everything -DNORSCODE_NATIVE_MAIN -o "$out_tmp" "$tmp" $_static_flag $_sqlite_flag $_static_extra_libs; then
        _clang_ec=$?
        rm -f "$tmp"
        rm -f "$out_tmp"
        tmp=""
        printf 'Feil: clang kompilering feila (exit %d)\n' "$_clang_ec" >&2
        return 1
    fi
    rm -f "$tmp"
    tmp=""
    trap - EXIT
    chmod +x "$out_tmp"

    if smoke_ok "$out_tmp"; then
        mv "$out_tmp" "$OUT"
    else
        rm -f "$out_tmp"
        case "$BOOTSTRAP_C_ROOT" in
            "$ROOT"/tools|tools)
                printf 'Feil: bygget binær feila NORSCODE_CMD-røyktest\n' >&2
                return 1
                ;;
            *)
                printf '⚠️  Historisk isolert C-output linka, men feila NORSCODE_CMD-røyktest; held på stage-0 seed.\n' >&2
                ;;
        esac
    fi
    printf "✓ dist/norscode_native bygget i historisk vedlikehaldsmodus frå %s/maint/c (%d bytes)\n" "$BOOTSTRAP_C_ROOT" "$(wc -c < "$OUT" | tr -d ' ')"
    return 0
}

append_historical_host_shims() {
    local out="$1"
    local gen="$2"

    cat >> "$out" <<'CEOF'

/* Historical regen compatibility shims.
 * Fresh ncb_to_c output can reference host builtin symbols directly. Keep these
 * in the explicit C maintenance lane instead of the normal .no -> NCB path. */
NcVal *nc_fn_builtin_ncb_route_handlers(NcVal **args, int na) { return nc_builtin_ncb_route_handlers(args, na); }
NcVal *nc_fn_builtin_ncb_call_fn(NcVal **args, int na) { return nc_builtin_ncb_call_fn(args, na); }

NcVal *nc_fn_builtin_tekst_til_liten(NcVal **args, int na) {
    char *s = nc_to_str_raw(na > 0 ? args[0] : nc_nil());
    for (char *p = s; p && *p; p++) {
        if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
    }
    NcVal *r = nc_str(s ? s : "");
    free(s);
    return r;
}

NcVal *nc_fn_builtin_tekst_til_heltall(NcVal **args, int na) {
    return nc_builtin_heltall(na > 0 ? args[0] : nc_nil());
}

NcVal *nc_fn_builtin_semantic_analyser(NcVal **args, int na) {
    return nc_dispatch_call("selfhost.compiler.semantic.semantic_analyser", args, na);
}
NcVal *nc_fn_builtin_semantic_analyser_rent(NcVal **args, int na) {
    return nc_fn_builtin_semantic_analyser(args, na);
}
NcVal *nc_fn_builtin_semantic_ok(NcVal **args, int na) {
    return nc_semantic_state_ok(na > 0 ? args[0] : nc_nil()) ? nc_bool(1) : nc_bool(0);
}
NcVal *nc_fn_builtin_semantic_rapport(NcVal **args, int na) {
    return nc_semantic_state_report(na > 0 ? args[0] : nc_nil());
}
NcVal *nc_fn_builtin_semantic_validate_program(NcVal **args, int na) {
    return nc_dispatch_call("semantic_validate_program", args, na);
}
NcVal *nc_fn_builtin_semantic_has_errors(NcVal **args, int na) {
    return nc_dispatch_call("semantic_has_errors", args, na);
}
NcVal *nc_fn_builtin_wasm_selftest(NcVal **args, int na) {
    (void)args; (void)na;
    return nc_bool(1);
}

static NcVal *nc_web_headers_from_ctx(NcVal *ctx) {
    if (!ctx || ctx->type != NC_MAP) return nc_map_new();
    NcVal *h = nc_index_get(ctx, nc_str("__headers__"));
    return (h && h->type == NC_MAP) ? h : nc_map_new();
}

NcVal *nc_fn_builtin_web_request_header(NcVal **args, int na) {
    if (na < 2) return nc_str("");
    NcVal *h = nc_web_headers_from_ctx(args[0]);
    char *key = nc_to_str_raw(args[1]);
    for (char *p = key; p && *p; p++) {
        if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
    }
    NcVal *v = nc_index_get(h, nc_str(key ? key : ""));
    free(key);
    return (v && v->type != NC_NIL) ? v : nc_str("");
}

NcVal *nc_fn_builtin_web_request_query_param(NcVal **args, int na) {
    if (na < 2 || !args[0] || args[0]->type != NC_MAP) return nc_str("");
    NcVal *q = nc_index_get(args[0], nc_str("__query__"));
    if (!q || q->type != NC_MAP) return nc_str("");
    char *key = nc_to_str_raw(args[1]);
    NcVal *v = nc_index_get(q, nc_str(key ? key : ""));
    free(key);
    return (v && v->type != NC_NIL) ? v : nc_str("");
}

NcVal *nc_fn_builtin_web_request_json(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_MAP) return nc_map_new();
    NcVal *body = nc_index_get(args[0], nc_str("__body__"));
    return nc_builtin_json_parse_raw(body && body->type == NC_STR ? body : nc_str("{}"));
}

NcVal *nc_fn_builtin_web_request_json_field(NcVal **args, int na) {
    if (na < 2) return nc_str("");
    NcVal *json = nc_fn_builtin_web_request_json(args, 1);
    if (!json || json->type != NC_MAP) return nc_str("");
    char *key = nc_to_str_raw(args[1]);
    NcVal *v = nc_index_get(json, nc_str(key ? key : ""));
    free(key);
    return (v && v->type != NC_NIL) ? v : nc_str("");
}

NcVal *nc_fn_builtin_web_response_builder(NcVal **args, int na) {
    NcVal *r = nc_map_new();
    nc_index_set(r, nc_str("__status__"), na > 0 ? args[0] : nc_int(200));
    nc_index_set(r, nc_str("__headers__"), na > 1 ? args[1] : nc_map_new());
    nc_index_set(r, nc_str("__body__"), na > 2 ? args[2] : nc_str(""));
    return r;
}

NcVal *nc_fn_builtin_web_response_error(NcVal **args, int na) {
    NcVal *status = na > 0 ? args[0] : nc_int(500);
    char *msg = nc_to_str_raw(na > 1 ? args[1] : nc_str("error"));
    size_t need = strlen(msg ? msg : "") + 32;
    char *body = malloc(need);
    snprintf(body, need, "{\"error\":\"%s\"}", msg ? msg : "");
    NcVal *headers = nc_map_new();
    nc_index_set(headers, nc_str("content-type"), nc_str("application/json"));
    NcVal *argv[] = { status, headers, nc_str(body) };
    NcVal *r = nc_fn_builtin_web_response_builder(argv, 3);
    free(msg);
    free(body);
    return r;
}

NcVal *nc_fn_builtin_web_has_role(NcVal **args, int na) {
    if (na < 2) return nc_bool(0);
    NcVal *role_v = nc_fn_builtin_web_request_header((NcVal *[]){args[0], nc_str("x-role")}, 2);
    char *got = nc_to_str_raw(role_v);
    char *want = nc_to_str_raw(args[1]);
    int ok = got && want && strcmp(got, want) == 0;
    free(got);
    free(want);
    return nc_bool(ok);
}

NcVal *nc_builtin_socket_new(NcVal *domain, NcVal *type) { (void)domain; (void)type; return nc_int(-1); }
NcVal *nc_builtin_socket_connect(NcVal *sock, NcVal *host, NcVal *port) { (void)sock; (void)host; (void)port; return nc_int(-1); }
NcVal *nc_builtin_socket_bind(NcVal *sock, NcVal *host, NcVal *port) { (void)sock; (void)host; (void)port; return nc_int(-1); }
NcVal *nc_builtin_socket_send(NcVal *sock, NcVal *data) { (void)sock; (void)data; return nc_int(-1); }
NcVal *nc_builtin_socket_send_bytes(NcVal *sock, NcVal *data) { (void)sock; (void)data; return nc_int(-1); }
NcVal *nc_builtin_socket_recv(NcVal *sock, NcVal *max) { (void)sock; (void)max; return nc_str(""); }
NcVal *nc_builtin_socket_recv_bytes(NcVal *sock, NcVal *max) { (void)sock; (void)max; return nc_str(""); }
NcVal *nc_builtin_socket_settimeout(NcVal *sock, NcVal *timeout) { (void)sock; (void)timeout; return nc_int(0); }

NcVal *nc_fn_std_socket_ny_tcp(NcVal **args, int na) { (void)args; (void)na; return nc_int(-1); }
NcVal *nc_fn_std_socket_lukk(NcVal **args, int na) { (void)args; (void)na; return nc_int(0); }
NcVal *nc_fn_std_socket_aksepter(NcVal **args, int na) { (void)args; (void)na; return nc_int(-1); }
NcVal *nc_fn_std_socket_motta(NcVal **args, int na) { (void)args; (void)na; return nc_str(""); }
NcVal *nc_fn_std_socket_send(NcVal **args, int na) { (void)args; (void)na; return nc_int(0); }
NcVal *nc_fn_std_socket_er_feil(NcVal **args, int na) { (void)args; (void)na; return nc_bool(1); }
NcVal *nc_fn_std_socket_bind(NcVal **args, int na) { (void)args; (void)na; return nc_int(-1); }
NcVal *nc_fn_std_socket_lytt(NcVal **args, int na) { (void)args; (void)na; return nc_int(-1); }
NcVal *nc_fn_std_socket_fil_deskriptor(NcVal **args, int na) { (void)args; (void)na; return nc_int(-1); }
CEOF

    if ! grep -q '^static NcVal \*nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_til_ncb_json(NcVal \*\*args, int nargs) {' "$gen"; then
        cat >> "$out" <<'CEOF'
NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_til_ncb_json(NcVal **args, int na) {
    return nc_dispatch_call("selfhost.compiler.ir_to_bytecode.kompiler_til_ncb_json", args, na);
}
CEOF
    fi
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
