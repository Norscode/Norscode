#!/usr/bin/env sh

set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
NC_BIN="$ROOT_DIR/bin/nc"
TEST_FILE="$ROOT_DIR/tests/test_helpdesk.no"

printf '=== Helpdesk API smoke (in-prosess) ===\n'
printf 'Bruker testkjerne i %s\n' "$TEST_FILE"

if ! [ -x "$NC_BIN" ]; then
    printf 'feil: manglar %s\n' "$NC_BIN" >&2
    exit 1
fi

if ! [ -f "$TEST_FILE" ]; then
    printf 'feil: manglar testfila %s\n' "$TEST_FILE" >&2
    exit 1
fi

set +e
"$NC_BIN" test "$TEST_FILE"
RESULT=$?
set -e

if [ "$RESULT" -ne 0 ]; then
    printf 'FEIL: helpdesk smoke feila (exit=%s)\n' "$RESULT" >&2
    exit "$RESULT"
fi

printf 'OK: helpdesk smoke bestod.\n'
