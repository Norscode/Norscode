#!/usr/bin/env bash

set -e

OUTPUT_DIR="reports/compiler_snapshots"
mkdir -p "$OUTPUT_DIR"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SNAPSHOT_DIR="$OUTPUT_DIR/snapshot_$TIMESTAMP"
mkdir -p "$SNAPSHOT_DIR"

echo "Creating compiler stage snapshot..."

# Copy available deterministic artifacts if present
for FILE in \
    build/compiler_stage1 \
    build/compiler_stage2 \
    reports/bootstrap_diff_report.txt
 do
    if [ -f "$FILE" ]; then
        cp "$FILE" "$SNAPSHOT_DIR/"
    fi
 done

# Capture hashes
{
    echo "=== Compiler Snapshot Hashes ==="
    echo ""
    find "$SNAPSHOT_DIR" -type f | while read -r FILE; do
        shasum -a 256 "$FILE"
    done
} > "$SNAPSHOT_DIR/hashes.txt"

echo "Compiler snapshot created at: $SNAPSHOT_DIR"
