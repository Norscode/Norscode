#!/bin/sh
# tools/nc_test.sh — tynn wrapper for Norscode-native testløpar
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
NC_TEST_FILE="${NC_TEST_FILE:-}"

if [ "$#" -ge 1 ] && [ "${1:-}" != "--no-color" ] && [ -f "${1:-}" ]; then
    NC_TEST_FILE="$1"
fi

if [ "${NC_CI:-0}" = "1" ] && [ "${NC_VERBOSE:-0}" = "0" ]; then
    NC_VERBOSE=1
    export NC_VERBOSE
fi

NC_TEST_FILES="${NC_TEST_FILES:-}"
if [ -z "$NC_TEST_FILE" ] && [ -z "$NC_TEST_FILES" ]; then
    NC_TEST_FILES="$(find "${TESTS_DIR:-$ROOT/tests}" -maxdepth 1 -type f -name 'test_*.no' | sort || true)"
fi

NC_NATIVE_PATH="${NC_NATIVE:-$ROOT/dist/norscode_native}"
if [ -x "$NC_NATIVE_PATH" ]; then
    NC_NATIVE_OK=1
else
    NC_NATIVE_OK=0
fi

_out="$(mktemp "${TMPDIR:-/tmp}/norscode_nc_test.XXXXXX")"
_rc=0
env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_ROOT="$ROOT" \
  NC_NATIVE="$NC_NATIVE_PATH" \
  NC_NATIVE_OK="$NC_NATIVE_OK" \
  HYBRID_COMPILE="${HYBRID_COMPILE:-$ROOT/tools/compile_with_hybrid_bundle_v9400.sh}" \
  TESTS_DIR="${TESTS_DIR:-$ROOT/tests}" \
  NC_TEST_TMPDIR="${NC_TEST_TMPDIR:-$ROOT/build/nc-test-tmp}" \
  NC_TEST_FILE="$NC_TEST_FILE" \
  NC_TEST_FILES="$NC_TEST_FILES" \
  "$ROOT/bin/nc" run "$ROOT/tools/nc_test.no" >"$_out" 2>&1 || _rc=$?

if [ "$_rc" -eq 0 ]; then
    cat "$_out"
    rm -f "$_out"
    exit 0
fi

if ! grep -Eq 'Ukjent funksjon: (builtin\.)?exec_prosess|Ukjent funksjon: builtin\.builtin\.exec_prosess' "$_out"; then
    cat "$_out" >&2
    rm -f "$_out"
    exit "$_rc"
fi
rm -f "$_out"

if [ "$NC_NATIVE_OK" != "1" ]; then
    printf 'FEIL: Trenger dist/norscode_native. Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

_is_slow() {
    case "$1" in
        test_chunk_*|test_file_io_large.no|test_selfhost.no) return 0 ;;
        *) return 1 ;;
    esac
}

_is_native_unsupported() {
    case "$1" in
        test_reactive*|test_islands*|test_html_state*|test_audit*|test_logging*|test_metrics*|test_json_schema*|test_state*|test_selfhost_bytecode*|test_selfhost_bridge*|test_snapshot*) return 0 ;;
        test_advanced_example.no|test_ai.no|test_api_advanced.no|test_api_server.no|test_api_simple_native.no|test_async_http.no|test_async_runtime.no|test_async_timeout.no|test_struct_example.no|test_wasm.no) return 0 ;;
        test_cache.no|test_chunk_2000.no|test_chunk_end.no|test_chunk_full.no|test_chunk_tail.no|test_dependency_import.no|test_http_helpers.no|test_helpdesk.no|test_ir_debug.no|test_nc_main_both.no|test_runtime_async_process_maturity.no) return 0 ;;
        test_selfhost_phase2_ffi_smoke.no|test_selfhost_phase2_regression.no|test_selfhost_phase2_smoke.no|test_selfhost_phase2_stdlib_usecases.no|test_stdlib_status_matrix.no) return 0 ;;
        test_frontend.no|test_html.no|test_html_components.no|test_html_components_v2.no|test_io_error.no|test_native_ui.no|test_native_ui_errors.no|test_secrets.no) return 0 ;;
        test_httpserver_vm_dispatch.no|test_httpserver_vm_health_log.no|test_httpserver_vm_response_helpers.no) return 0 ;;
        test_selfhost_ifexpr_elif.no|test_selfhost_ifexpr_v21.no|test_selfhost_phase3_ir_regression.no|test_selfhost_phase3_ir_smoke.no|test_selfhost_phase3_regression.no|test_selfhost_phase3_smoke.no|test_selfhost_phase4_regression.no|test_selfhost_phase4_smoke.no|test_selfhost_phase5_smoke.no|test_selfhost_phase6_regression.no|test_selfhost_phase6_smoke.no) return 0 ;;
        test_web_api_versioning.no|test_web_api_versioning_example.no|test_web_auth.no|test_csrf.no|test_web_dependency.no|test_web_cookies.no|test_web_openapi.no|test_web_openapi_auth.no|test_web_openapi_errors.no|test_web_openapi_schema.no|test_web_request_response.no|test_web_path.no|test_web_roles.no|test_web_sanitize.no|test_web_dependency_example.no|test_web_example.no|test_web_handle_request_fallback.no|test_web_roles_example.no|test_web_validation.no|test_web_validation_example.no) return 0 ;;
        test_security.no|test_stil.no|test_selfhost.no) return 0 ;;
        *) return 1 ;;
    esac
}

_run_one() {
    _file="$1"
    env NORSCODE_CMD=run NORSCODE_FILE="$_file" NORSCODE_ROOT="$ROOT" "$NC_NATIVE_PATH"
}

pass=0
fail=0
skip=0
total=0

if [ -n "$NC_TEST_FILE" ]; then
    printf 'Testing: %s\n' "$NC_TEST_FILE"
    total=1
    if _run_one "$NC_TEST_FILE"; then
        pass=1
    else
        fail=1
    fi
else
    printf 'Norscode Python-fri testløpar\n'
    printf 'Kompilerer og køyrer tests/test_*.no via norscode_native...\n\n'
    if [ -z "$NC_TEST_FILES" ]; then
        printf 'FEIL: Ingen testfiler funne i %s\n' "${TESTS_DIR:-$ROOT/tests}" >&2
        exit 1
    fi
    for _file in $NC_TEST_FILES; do
        [ -n "$_file" ] || continue
        _name="$(basename "$_file")"
        if [ "${NC_SLOW_TESTS:-0}" = "1" ] && ! _is_slow "$_name"; then
            continue
        fi
        total=$((total + 1))
        if _is_native_unsupported "$_name"; then
            skip=$((skip + 1))
            continue
        fi
        if _run_one "$_file" >/dev/null 2>&1; then
            pass=$((pass + 1))
            if [ "${NC_VERBOSE:-0}" = "1" ]; then
                printf '  OK %s\n' "${_name%.no}"
            else
                printf '.'
            fi
        else
            fail=$((fail + 1))
            printf '\n  FEIL %s\n' "${_name%.no}"
        fi
    done
    printf '\n'
fi

printf '\n--- Resultat ------------------------------\n'
printf '  Bestått:  %s\n' "$pass"
printf '  Feilet:   %s\n' "$fail"
printf '  Hoppa:    %s  (native-unsupported eller slow)\n' "$skip"
printf '  Totalt:   %s\n\n' "$total"

if [ "$fail" -gt 0 ]; then
    printf 'FEIL: Testane feilet (%s av %s)\n' "$fail" "$((pass + fail))"
    exit 1
fi
printf 'OK: Alle testar bestått (%s/%s)\n' "$pass" "$((pass + fail))"
