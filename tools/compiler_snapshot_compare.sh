#!/usr/bin/env bash

set -e

if [ $# -lt 2 ]; then
    echo "Usage: compiler_snapshot_compare.sh <snapshot_dir_a> <snapshot_dir_b>"
    exit 1
fi

SNAPSHOT_A="$1"
SNAPSHOT_B="$2"

if [ ! -d "$SNAPSHOT_A" ]; then
    echo "Missing snapshot directory: $SNAPSHOT_A"
    exit 1
fi

if [ ! -d "$SNAPSHOT_B" ]; then
    echo "Missing snapshot directory: $SNAPSHOT_B"
    exit 1
fi

mkdir -p reports
REPORT="reports/compiler_snapshot_compare_report.txt"

{
    echo "=== Compiler Snapshot Comparison ==="
    echo ""
    echo "Snapshot A: $SNAPSHOT_A"
    echo "Snapshot B: $SNAPSHOT_B"
    echo ""

    echo "File inventory comparison:"
    diff -qr "$SNAPSHOT_A" "$SNAPSHOT_B" || true

    echo ""
    echo "Hash comparison:"

    find "$SNAPSHOT_A" -type f | sort | while read -r FILE_A; do
        RELATIVE=$(realpath --relative-to="$SNAPSHOT_A" "$FILE_A")
        FILE_B="$SNAPSHOT_B/$RELATIVE"

        if [ -f "$FILE_B" ]; then
            HASH_A=$(shasum -a 256 "$FILE_A" | awk '{print $1}')
            HASH_B=$(shasum -a 256 "$FILE_B" | awk '{print $1}')

            if [ "$HASH_A" = "$HASH_B" ]; then
                echo "MATCH $RELATIVE"
            else
                echo "DIFF  $RELATIVE"
            fi
        else
            echo "MISSING IN SNAPSHOT_B: $RELATIVE"
        fi
    done
} > "$REPORT"

echo "Snapshot comparison report written to: $REPORT"
