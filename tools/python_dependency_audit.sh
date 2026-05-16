#!/usr/bin/env bash

set -e

echo "Scanning Python dependencies in Norscode..."

OUTPUT_DIR="reports"
OUTPUT_FILE="$OUTPUT_DIR/python_dependency_report.txt"

mkdir -p "$OUTPUT_DIR"

{
    echo "=== Norscode Python Dependency Report ==="
    echo ""

    echo "Python files count:"
    find . -name "*.py" | wc -l

    echo ""
    echo "Python file list:"
    find . -name "*.py" | sort

    echo ""
    echo "Potential compiler-critical files:"
    find . -name "*.py" | grep -E "parser|semantic|compiler|backend|runtime|optimizer|bootstrap" || true

    echo ""
    echo "Temporary/debug Python files:"
    find . -name "*.py" | grep -E "tmp|debug|old|backup" || true

} > "$OUTPUT_FILE"

echo "Dependency audit written to: $OUTPUT_FILE"
