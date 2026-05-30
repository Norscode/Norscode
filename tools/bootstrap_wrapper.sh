#!/usr/bin/env sh
# dist/norcode-bootstrap-compile — shell-wrapper rundt nc-vm
# AVVIKLA: Bruk dist/norscode_native i staden.
# Dette er berre ein bakoverkompatibel wrapper.
# Handterar NORCODE_BOOTSTRAP_* env-var-protokollen.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NC_VM="$ROOT/dist/nc-vm"

if [ ! -x "$NC_VM" ]; then
    printf 'norcode: nc-vm ikkje funnen: %s\n' "$NC_VM" >&2
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
        exec "$NC_VM" --nc-run "$SRC"
    fi
    if [ "$CMD" = "build" ] && [ -n "$SRC" ] && [ -n "$OUT" ]; then
        exec "$NC_VM" --nc-compile "$SRC" "$OUT"
    fi
    printf 'norcode: ugyldig NORCODE_CMD=%s\n' "$CMD" >&2
    exit 1
fi

exec "$NC_VM" "$@"
