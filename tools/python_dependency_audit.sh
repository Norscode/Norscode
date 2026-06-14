#!/usr/bin/env bash
# tools/python_dependency_audit.sh — Python-audit i rein shell for robust sjølvstendighetsgate

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OUTPUT_DIR="reports"
OUTPUT_FILE="$OUTPUT_DIR/python_dependency_report.txt"
mkdir -p "$OUTPUT_DIR"

TOOLS_PY="$(rg --files tools 2>/dev/null | rg '\.py$' || true)"
REPO_PY="$(rg --files -g '*.py' 2>/dev/null || true)"

count_lines() {
    local data="$1"
    if [ -z "$data" ]; then
        printf '0'
    else
        printf '%s\n' "$data" | wc -l | tr -d ' '
    fi
}

TOOLS_COUNT="$(count_lines "$TOOLS_PY")"
REPO_COUNT="$(count_lines "$REPO_PY")"

{
    printf '=== Norscode Python Dependency Report ===\n\n'
    printf 'Python files in tools/ (skal vere 0 i normal flyt): %s\n' "$TOOLS_COUNT"
    if [ -n "$TOOLS_PY" ]; then
        printf '%s\n' "$TOOLS_PY"
    fi
    printf '\nPython files in repo (ekskl. .git): %s\n' "$REPO_COUNT"
    if [ -n "$REPO_PY" ]; then
        printf '%s\n' "$REPO_PY"
    fi
    printf '\nTip: Bruk bash tools/no_c_python_active_surface.sh for full normal/legacy-surface policy.\n'
    printf 'Note: rapporten kan innehalde Python i archive/scripts som er tillate per policy.\n'
} > "$OUTPUT_FILE"

printf 'Dependency audit written to: %s\n' "$OUTPUT_FILE"

if [ "$TOOLS_COUNT" != "0" ]; then
    printf 'Feil: Python i tools/ er ikkje tillatt i normal flyt. Sjå %s\n' "$OUTPUT_FILE" >&2
    exit 1
fi

printf 'OK: ingen Python i tools/\n'
