#!/usr/bin/env sh
# dist/norscode-bootstrap-compile — shell-wrapper for bakoverkompatibilitet
# Delegerer til dist/norscode_native (ikkje nc-vm).
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NC_NATIVE="$ROOT/dist/norscode_native"

if [ ! -x "$NC_NATIVE" ]; then
    printf 'norscode: norscode_native ikkje funnen: %s\n' "$NC_NATIVE" >&2
    printf 'Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

if [ -n "${NORCODE_BOOTSTRAP_CLI:-}" ]; then
    ARGC="${NORCODE_ARGC:-1}"
    CMD="${NORCODE_ARG0:-selfcheck}"
    case "$CMD" in
        selfcheck|test|identity|release|release-targets)
            exec sh "$ROOT/bin/nc" selfcheck
            ;;
        help|--help|-h)
            exec sh "$ROOT/bin/nc" help
            ;;
        *)
            exec sh "$ROOT/bin/nc" "$CMD"
            ;;
    esac
fi

if [ -n "${NORCODE_BOOTSTRAP_VM:-}" ]; then
    CMD="${NORCODE_CMD:-run}"
    SRC="${NORCODE_SRC:-}"
    OUT="${NORCODE_OUT:-}"
    if [ "$CMD" = "run" ] && [ -n "$SRC" ]; then
        exec env NORSCODE_CMD=run NORSCODE_FILE="$SRC" "$NC_NATIVE"
    fi
    if [ "$CMD" = "build" ] && [ -n "$SRC" ] && [ -n "$OUT" ]; then
        exec env NORSCODE_CMD=compile NORSCODE_FILE="$SRC" NORSCODE_OUTPUT="$OUT" "$NC_NATIVE"
    fi
    printf 'norscode: ugyldig NORCODE_CMD=%s\n' "$CMD" >&2; exit 1
fi

# Direkte kall: deleger til norscode_native
exec env NORSCODE_CMD=run NORSCODE_FILE="${1:-}" "$NC_NATIVE"
