#!/usr/bin/env bash
# tools/python_dependency_audit.sh — rapport + gate for Python i normal flate

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OUTPUT_DIR="reports"
OUTPUT_FILE="$OUTPUT_DIR/python_dependency_report.txt"
mkdir -p "$OUTPUT_DIR"

TOOLS_PY="$(find tools -maxdepth 1 -name '*.py' 2>/dev/null | sort || true)"
TOOLS_PY_COUNT="$(printf '%s\n' "$TOOLS_PY" | sed '/^$/d' | wc -l | tr -d ' ')"

{
    echo "=== Norscode Python Dependency Report ==="
    echo ""
    echo "Python files in tools/ (skal vere 0 i normal flyt): $TOOLS_PY_COUNT"
    if [ "$TOOLS_PY_COUNT" != "0" ]; then
        echo "$TOOLS_PY"
    fi
    echo ""
    echo "All .py in repo (ekskl. .git):"
    find . -path './.git' -prune -o -name '*.py' -print | sort
    echo ""
    echo "Count:"
    find . -path './.git' -prune -o -name '*.py' -print | wc -l
} > "$OUTPUT_FILE"

echo "Dependency audit written to: $OUTPUT_FILE"

if [ "$TOOLS_PY_COUNT" != "0" ]; then
    printf 'Feil: Python i tools/ er ikkje tillatt i normal flyt. Sjå %s\n' "$OUTPUT_FILE" >&2
    exit 1
fi

printf 'OK: ingen Python i tools/\n'
