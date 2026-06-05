#!/bin/sh
# tools/nc_test.sh — Python-fri testløpar for Norscode
#
# Køyrer alle tests/test_*.no via norscode_native (eller nc-vm som fallback)
# og rapporterer pass/fail med farga output.
#
# Bruk:
#   sh tools/nc_test.sh
#   sh tools/nc_test.sh tests/test_math.no      # ein spesifikk test
#   sh tools/nc_test.sh --no-color               # utan farge
#   NC_VERBOSE=1 sh tools/nc_test.sh             # vis output frå kvar test

set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

NC_NATIVE="${NC_NATIVE:-$ROOT/dist/norscode_native}"
TESTS_DIR="${TESTS_DIR:-$ROOT/tests}"
NC_VERBOSE="${NC_VERBOSE:-0}"
TIMEOUT="${TEST_TIMEOUT:-30}"
NC_TEST_SHARD="${NC_TEST_SHARD:-}"
NC_TEST_SHARDS="${NC_TEST_SHARDS:-}"

if [ "${NC_CI:-0}" = "1" ] && [ "$NC_VERBOSE" = "0" ]; then
    NC_VERBOSE=1
fi

use_shard() {
    [ -n "$NC_TEST_SHARD" ] && [ -n "$NC_TEST_SHARDS" ] || return 1
    [ "$NC_TEST_SHARDS" -gt 1 ] 2>/dev/null || return 1
    return 0
}

# Farge-støtte
if [ -t 1 ] && [ "${1:-}" != "--no-color" ]; then
    GRN='\033[0;32m'; RED='\033[0;31m'; YLW='\033[0;33m'
    BLD='\033[1m'; RST='\033[0m'
else
    GRN=''; RED=''; YLW=''; BLD=''; RST=''
fi

# Velg beste runner
if [ -x "$NC_NATIVE" ]; then
    _NC_RUN="env NORSCODE_CMD=run NORSCODE_FILE="
    _NC_RUNNER="$NC_NATIVE"
elif [ -x "$NC_VM" ]; then
    printf '%sAdvarsel: brukar nc-vm (eldre). Bygg norscode_native for betre ytelse.%s\n' "$YLW" "$RST" >&2
    _NC_RUN="$NC_VM --nc-run "
    _NC_RUNNER=""
else
    printf '%s✗ Trenger dist/norscode_native eller dist/nc-vm. Køyr: bash tools/build_norscode_native.sh%s\n' "$RED" "$RST" >&2
    exit 1
fi

run_with_timeout() {
    if command -v timeout >/dev/null 2>&1; then
        timeout "$TIMEOUT" "$@"
        return $?
    fi

    if command -v perl >/dev/null 2>&1; then
        perl -e '
            my $t = shift @ARGV;
            alarm $t;
            exec @ARGV or die "exec failed: $!\n";
        ' "$TIMEOUT" "$@"
        return $?
    fi

    "$@"
}

# ─── Tester som krev runtimeflater som ikkje er i rask native-runner enno
is_server_test() {
    case "$(basename "$1")" in
        test_async*|test_reactive*|test_islands*|\
        test_html_state*|test_audit*|test_db*|\
        test_logging*|test_metrics*|\
        test_json_schema*|test_state*|\
        test_selfhost_bytecode*|test_selfhost_bridge*|\
        test_snapshot*|\
        test_cache.no|test_chunk_2000.no|\
        test_chunk_end.no|test_chunk_full.no|test_chunk_tail.no|\
        test_dependency_import.no|\
        test_file_object_storage.no|test_http_helpers.no|\
        test_ir_debug.no|\
        test_nc_main_both.no|\
        test_ny_liste.no|\
        test_security.no|\
        test_selfhost.no)
            return 0;;
    esac
    return 1
}

# ─── CI: parser-stress / monolitt (køyr i eiga jobb, sjå native-*-slow i ci.yml)
is_slow_test() {
    case "$(basename "$1")" in
        test_chunk_*|test_selfhost.no)
            return 0;;
    esac
    return 1
}

# ─── Tellorar
pass=0; fail=0; skip=0; total=0

run_test() {
    _file="$1"
    _name="$(basename "$_file" .no)"

    if [ "${NC_SLOW_TESTS:-0}" = "1" ] && ! is_slow_test "$_file"; then
        return
    fi

    total=$((total + 1))

    if [ "${NC_SLOW_TESTS:-0}" != "1" ] && is_slow_test "$_file"; then
        skip=$((skip + 1))
        if [ "${NC_VERBOSE:-0}" = "2" ]; then
            printf '  %s⊘ skipped (slow):%s %s\n' "$YLW" "$RST" "$_name"
        fi
        return
    elif is_server_test "$_file"; then
        skip=$((skip + 1))
        if [ "${NC_VERBOSE:-0}" = "2" ]; then
            printf '  %s⊘ skipped (native-unsupported):%s %s\n' "$YLW" "$RST" "$_name"
        fi
        return
    fi

    if [ "${NC_VERBOSE:-0}" = "1" ]; then
        printf '  → %s\n' "$_name"
    fi

    if [ -n "$_NC_RUNNER" ]; then
        if _out=$(run_with_timeout env NORSCODE_CMD=run NORSCODE_FILE="$_file" "$_NC_RUNNER" 2>&1); then
            _ec=0
        else
            _ec=$?
        fi
    else
        if _out=$(${_NC_RUN}"$_file" 2>&1); then
            _ec=0
        else
            _ec=$?
        fi
    fi

    if [ "$_ec" -eq 0 ]; then
        pass=$((pass + 1))
        if [ "${NC_VERBOSE:-0}" = "1" ]; then
            printf '  %s✓%s %s\n' "$GRN" "$RST" "$_name"
            if [ -n "$_out" ] && [ "${NC_VERBOSE:-0}" = "2" ]; then
                printf '%s\n' "$_out" | sed 's/^/    /'
            fi
        else
            printf '.'
        fi
    else
        fail=$((fail + 1))
        _msg=$(printf '%s' "$_out" | grep -v '^\[nc-vm\]' | grep -v '^\[norscode\]' | grep -v '^$' | head -1)
        printf '\n  %s✗%s %s\n    %s\n' "$RED" "$RST" "$_name" "$_msg"
    fi
}

# ─── Køyr testar
if [ $# -ge 1 ] && [ "${1:-}" != "--no-color" ] && [ -f "${1:-}" ]; then
    # Enkelt test-fil
    printf '%sTesting: %s%s\n' "$BLD" "$1" "$RST"
    run_test "$1"
else
    printf '%sNorscode Python-fri testløpar%s\n' "$BLD" "$RST"
    if use_shard; then
        printf 'Shard: %s/%s\n' "$NC_TEST_SHARD" "$NC_TEST_SHARDS"
    fi
    printf 'Kompilerer og køyrer tests/test_*.no via norscode_native...\n\n'

    _idx=0
    for _f in "$TESTS_DIR"/test_*.no; do
        [ -f "$_f" ] || continue
        if use_shard; then
            _mod=$(( _idx % NC_TEST_SHARDS ))
            _idx=$(( _idx + 1 ))
            [ "$_mod" -eq "$NC_TEST_SHARD" ] || continue
        fi
        run_test "$_f"
    done
    printf '\n'
fi

# ─── Oppsummering
printf '\n%s─── Resultat ──────────────────────────────%s\n' "$BLD" "$RST"
printf '  Bestått:  %s%d%s\n' "$GRN" "$pass" "$RST"
if [ "$fail" -gt 0 ]; then
    printf '  Feilet:   %s%d%s\n' "$RED" "$fail" "$RST"
else
    printf '  Feilet:   %d\n' "$fail"
fi
printf '  Hoppa:    %d  (native-unsupported eller slow)\n' "$skip"
printf '  Totalt:   %d\n' "$total"
printf '\n'

if [ "$fail" -gt 0 ]; then
    printf '%s✗ Testane FEILET%s (%d av %d)\n' "$RED$BLD" "$RST" "$fail" "$((pass + fail))" >&2
    exit 1
else
    printf '%s✓ Alle testar bestått%s (%d/%d)\n' "$GRN$BLD" "$RST" "$pass" "$((pass + fail))"
    exit 0
fi
