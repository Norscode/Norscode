#!/usr/bin/env bash

set -e

if [ $# -lt 2 ]; then
    echo "Usage: symbol_table_serializer.sh <input_symbols> <output_symbols>"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"

if [ ! -f "$INPUT_FILE" ]; then
    echo "Missing symbol dump: $INPUT_FILE"
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT_FILE")"

# Normalize symbol ordering and whitespace
cat "$INPUT_FILE" \
    | sed 's/[[:space:]]\+/ /g' \
    | sed 's/[[:space:]]*$//' \
    | LC_ALL=C sort \
    > "$OUTPUT_FILE"

HASH=$(shasum -a 256 "$OUTPUT_FILE" | awk '{print $1}')

echo "Deterministic symbol table written to: $OUTPUT_FILE"
echo "Deterministic symbol hash: $HASH"
