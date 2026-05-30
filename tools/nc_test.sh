#!/bin/sh
# tools/nc_test.sh — Python-fri testløpar for Norscode
#
# Køyrer alle tests/test_*.no via dist/nc-vm --nc-run
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

NC_VM="${NC_VM:-$ROOT/dist/nc-vm}"
TESTS_DIR="${TESTS_DIR:-$ROOT/tests}"
NC_VERBOSE="${NC_VERBOSE:-0}"
TIMEOUT="${TEST_TIMEOUT:-30}"

# Farge-støtte
if [ -t 1 ] && [ "${1:-}" != "--no-color" ]; then
    GRN='\033[0;32m'; RED='\033[0;31m'; YLW='\033[0;33m'
    BLD='\033[1m'; RST='\033[0m'
else
    GRN=''; RED=''; YLW=''; BLD=''; RST=''
fi

# Sjekk at nc-vm er tilgjengeleg
if [ ! -x "$NC_VM" ]; then
    printf '%s✗ dist/nc-vm ikkje funnen. Køyr: sh tools/bootstrap.sh%s\n' "$RED" "$RST" >&2
    exit 1
fi

# ─── Tester som krev Python (test av Python-spesifikke selfhost.compiler-alias)
PYTHON_ONLY_TESTS="test_selfhost.no"

is_python_only() {
    _base="$(basename "$1")"
    for _pt in $PYTHON_ONLY_TESTS; do
        if [ "$_base" = "$_pt" ]; then return 0; fi
    done
    return 1
}

# ─── Tester som krev server/async/web (ikkje støtta av nc-vm --nc-run)
is_server_test() {
    case "$(basename "$1")" in
        test_web*|test_async*|test_reactive*|test_islands*|test_frontend*|\
        test_html*|test_csrf*|test_audit*|test_storage*|test_db*|\
        test_path_env*|test_logging*|test_metrics*|test_io_error*|\
        test_secrets*|test_json_schema*|test_state*|test_native_*|\
        test_selfhost_bytecode*|test_selfhost_bridge*|test_selfhost_ast*|\
        test_selfhost_comparison*|test_selfhost_boolean_parity*|\
        test_snapshot*|test_boolean_precedence*|test_trace*|test_comprehension*)
            return 0;;
    esac
    return 1
}

# ─── Tellorar
pass=0; fail=0; skip=0; total=0

run_test() {
    _file="$1"
    _name="$(basename "$_file" .no)"
    total=$((total + 1))

    if is_python_only "$_file"; then
        skip=$((skip + 1))
        printf '  %s⊘ skipped (Python-only):%s %s\n' "$YLW" "$RST" "$_name"
        return
    fi

    if is_server_test "$_file"; then
        skip=$((skip + 1))
        if [ "${NC_VERBOSE:-0}" = "2" ]; then
            printf '  %s⊘ skipped (server/async):%s %s\n' "$YLW" "$RST" "$_name"
        fi
        return
    fi

    _out=$("$NC_VM" --nc-run "$_file" 2>&1) || true
    _ec=$?

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
        _msg=$(printf '%s' "$_out" | grep -v '^\[nc-vm\]' | grep -v '^$' | head -1)
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
    printf 'Kompilerer og køyrer tests/test_*.no via dist/nc-vm...\n\n'

    for _f in "$TESTS_DIR"/test_*.no; do
        [ -f "$_f" ] || continue
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
printf '  Hoppa:    %d  (server/async/Python-only)\n' "$skip"
printf '  Totalt:   %d\n' "$total"
printf '\n'

if [ "$fail" -gt 0 ]; then
    printf '%s✗ Testane FEILET%s (%d av %d)\n' "$RED$BLD" "$RST" "$fail" "$((pass + fail))" >&2
    exit 1
else
    printf '%s✓ Alle testar bestått%s (%d/%d)\n' "$GRN$BLD" "$RST" "$pass" "$((pass + fail))"
    exit 0
fi
