#!/usr/bin/env bash

set -e

echo "Starting Norscode bootstrap verification..."

STAGE1_OUT="build/compiler_stage1"
STAGE2_OUT="build/compiler_stage2"

mkdir -p build

# Stage 1 build
./bin/bootstrap build compiler > "$STAGE1_OUT"

# Stage 2 build
./bin/bootstrap build compiler > "$STAGE2_OUT"

HASH1=$(shasum -a 256 "$STAGE1_OUT" | awk '{print $1}')
HASH2=$(shasum -a 256 "$STAGE2_OUT" | awk '{print $1}')

echo "Stage1 hash: $HASH1"
echo "Stage2 hash: $HASH2"

if [ "$HASH1" = "$HASH2" ]; then
    echo "Bootstrap verification PASSED"
    exit 0
else
    echo "Bootstrap verification FAILED"
    exit 1
fi
