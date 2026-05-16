#!/usr/bin/env bash

set -e

if [ $# -lt 2 ]; then
    echo "Usage: ast_canonical_dump.sh <input_ast_dump> <output_dump>"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"

if [ ! -f "$INPUT_FILE" ]; then
    echo "Missing AST dump: $INPUT_FILE"
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT_FILE")"

# Normalize whitespace and ordering-sensitive formatting
cat "$INPUT_FILE" \
    | sed 's/[[:space:]]\+/ /g' \
    | sed 's/[[:space:]]*$//' \
    | LC_ALL=C sort \
    > "$OUTPUT_FILE"

HASH=$(shasum -a 256 "$OUTPUT_FILE" | awk '{print $1}')

echo "Canonical AST dump written to: $OUTPUT_FILE"
echo "Canonical AST hash: $HASH"
