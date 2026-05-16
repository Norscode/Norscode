#!/usr/bin/env bash

set -e

if [ $# -lt 2 ]; then
    echo "Usage: compare_compilers.sh <compiler_a> <compiler_b>"
    exit 1
fi

COMPILER_A="$1"
COMPILER_B="$2"

if [ ! -f "$COMPILER_A" ]; then
    echo "Missing compiler: $COMPILER_A"
    exit 1
fi

if [ ! -f "$COMPILER_B" ]; then
    echo "Missing compiler: $COMPILER_B"
    exit 1
fi

HASH_A=$(shasum -a 256 "$COMPILER_A" | awk '{print $1}')
HASH_B=$(shasum -a 256 "$COMPILER_B" | awk '{print $1}')

echo "Compiler A: $COMPILER_A"
echo "Compiler B: $COMPILER_B"

echo ""
echo "Hash A: $HASH_A"
echo "Hash B: $HASH_B"

echo ""

if [ "$HASH_A" = "$HASH_B" ]; then
    echo "Compiler equivalence PASSED"
    exit 0
else
    echo "Compiler equivalence FAILED"
    echo "Differences detected between compiler outputs"
    exit 1
fi
