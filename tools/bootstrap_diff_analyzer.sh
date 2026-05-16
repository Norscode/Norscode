#!/usr/bin/env bash

set -e

if [ $# -lt 2 ]; then
    echo "Usage: bootstrap_diff_analyzer.sh <build_a> <build_b>"
    exit 1
fi

BUILD_A="$1"
BUILD_B="$2"

if [ ! -f "$BUILD_A" ]; then
    echo "Missing build artifact: $BUILD_A"
    exit 1
fi

if [ ! -f "$BUILD_B" ]; then
    echo "Missing build artifact: $BUILD_B"
    exit 1
fi

mkdir -p reports
REPORT="reports/bootstrap_diff_report.txt"

{
    echo "=== Norscode Bootstrap Diff Analyzer ==="
    echo ""
    echo "Build A: $BUILD_A"
    echo "Build B: $BUILD_B"
    echo ""

    HASH_A=$(shasum -a 256 "$BUILD_A" | awk '{print $1}')
    HASH_B=$(shasum -a 256 "$BUILD_B" | awk '{print $1}')

    echo "Hash A: $HASH_A"
    echo "Hash B: $HASH_B"
    echo ""

    if [ "$HASH_A" = "$HASH_B" ]; then
        echo "Bootstrap outputs are deterministic"
    else
        echo "Bootstrap mismatch detected"
        echo ""
        echo "First binary differences:"
        cmp -l "$BUILD_A" "$BUILD_B" | head -50 || true
    fi
} > "$REPORT"

echo "Bootstrap diff report written to: $REPORT"
