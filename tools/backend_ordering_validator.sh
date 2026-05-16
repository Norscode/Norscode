#!/usr/bin/env bash

set -e

if [ $# -lt 2 ]; then
    echo "Usage: backend_ordering_validator.sh <backend_output_a> <backend_output_b>"
    exit 1
fi

FILE_A="$1"
FILE_B="$2"

if [ ! -f "$FILE_A" ]; then
    echo "Missing backend output: $FILE_A"
    exit 1
fi

if [ ! -f "$FILE_B" ]; then
    echo "Missing backend output: $FILE_B"
    exit 1
fi

mkdir -p reports
REPORT="reports/backend_ordering_report.txt"

{
    echo "=== Backend Ordering Validator ==="
    echo ""
    echo "Backend Output A: $FILE_A"
    echo "Backend Output B: $FILE_B"
    echo ""

    HASH_A=$(shasum -a 256 "$FILE_A" | awk '{print $1}')
    HASH_B=$(shasum -a 256 "$FILE_B" | awk '{print $1}')

    echo "Hash A: $HASH_A"
    echo "Hash B: $HASH_B"
    echo ""

    if [ "$HASH_A" = "$HASH_B" ]; then
        echo "Backend ordering is deterministic"
    else
        echo "Backend ordering mismatch detected"
        echo ""
        echo "Unified diff:"
        diff -u "$FILE_A" "$FILE_B" || true
    fi
} > "$REPORT"

echo "Backend ordering report written to: $REPORT"
