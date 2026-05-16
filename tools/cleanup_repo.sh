#!/usr/bin/env bash

set -e

echo "Starting Norscode repository cleanup..."

# Python cache
find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
find . -type f -name "*.pyc" -delete 2>/dev/null || true
find . -type f -name "*.pyo" -delete 2>/dev/null || true

# macOS files
find . -type f -name ".DS_Store" -delete 2>/dev/null || true
find . -type d -name "__MACOSX" -exec rm -rf {} + 2>/dev/null || true

# Temp/debug files
find . -type f -name "tmp_*" -delete 2>/dev/null || true
find . -type f -name "debug_*" -delete 2>/dev/null || true
find . -type f -name "*.tmp" -delete 2>/dev/null || true
find . -type f -name "*.bak" -delete 2>/dev/null || true
find . -type f -name "*.old" -delete 2>/dev/null || true

# Logs
find . -type f -name "*.log" -delete 2>/dev/null || true

# Build artifacts
rm -rf build dist release artifacts 2>/dev/null || true

# Empty directories
find . -type d -empty -delete 2>/dev/null || true

echo "Cleanup complete."
