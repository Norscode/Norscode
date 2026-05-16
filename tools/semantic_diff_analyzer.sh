#!/usr/bin/env bash

set -e

if [ $# -lt 2 ]; then
    echo "Usage: semantic_diff_analyzer.sh <symbols_a> <symbols_b>"
    exit 1
fi

FILE_A="$1"
FILE_B="$2"

if [ ! -f "$FILE_A" ]; then
    echo "Missing semantic dump: $FILE_A"
    exit 1
fi

if [ ! -f "$FILE_B" ]; then
    echo "Missing semantic dump: $FILE_B"
    exit 1
fi

mkdir -p reports
REPORT="reports/semantic_diff_report.txt"

{
    echo "=== Norscode Semantic Diff Analyzer ==="
    echo ""
    echo "Semantic Dump A: $FILE_A"
    echo "Semantic Dump B: $FILE_B"
    echo ""

    HASH_A=$(shasum -a 256 "$FILE_A" | awk '{print $1}')
    HASH_B=$(shasum -a 256 "$FILE_B" | awk '{print $1}')

    echo "Hash A: $HASH_A"
    echo "Hash B: $HASH_B"
    echo ""

    if [ "$HASH_A" = "$HASH_B" ]; then
        echo "Semantic ordering is deterministic"
    else
        echo "Semantic ordering mismatch detected"
        echo ""
        echo "Unified diff:"
        diff -u "$FILE_A" "$FILE_B" || true
    fi
} > "$REPORT"

echo "Semantic diff report written to: $REPORT"
