#!/usr/bin/env bash

set -e

ARTIFACT_DIR="reports/bootstrap_artifacts"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$ARTIFACT_DIR/run_$TIMESTAMP"

mkdir -p "$RUN_DIR"

echo "Collecting bootstrap artifacts..."

collect_if_exists() {
    FILE="$1"

    if [ -e "$FILE" ]; then
        cp -R "$FILE" "$RUN_DIR/"
    fi
}

collect_if_exists reports/bootstrap_diff_report.txt
collect_if_exists reports/semantic_diff_report.txt
collect_if_exists reports/backend_ordering_report.txt
collect_if_exists reports/compiler_snapshot_compare_report.txt
collect_if_exists reports/compiler_snapshots
collect_if_exists build/compiler_stage1
collect_if_exists build/compiler_stage2
collect_if_exists build/compiler_stage3

{
    echo "=== Bootstrap Artifact Inventory ==="
    echo ""
    find "$RUN_DIR" -type f | sort
    echo ""
    echo "=== Artifact Hashes ==="
    echo ""
    find "$RUN_DIR" -type f | sort | while read -r FILE; do
        shasum -a 256 "$FILE"
    done
} > "$RUN_DIR/artifact_manifest.txt"

echo "Bootstrap artifact pipeline completed"
echo "Artifacts stored in: $RUN_DIR"
