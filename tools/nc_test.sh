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

exec env \
  NORSCODE_ENABLE_EXEC_PROSESS=1 \
  NORSCODE_ROOT="$ROOT" \
  NC_NATIVE="${NC_NATIVE:-$ROOT/dist/norscode_native}" \
  HYBRID_COMPILE="${HYBRID_COMPILE:-$ROOT/tools/compile_with_hybrid_bundle_v9400.sh}" \
  TESTS_DIR="${TESTS_DIR:-$ROOT/tests}" \
  NC_TEST_TMPDIR="${NC_TEST_TMPDIR:-$ROOT/build/nc-test-tmp}" \
  NC_TEST_FILE="$NC_TEST_FILE" \
  "$ROOT/bin/nc" run "$ROOT/tools/nc_test.no"
