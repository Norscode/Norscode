#!/usr/bin/env bash
set -e

echo "Cleaning repository artifacts..."

find . -name "__pycache__" -type d -exec rm -rf {} + || true
find . -name "*.pyc" -delete || true
find . -name "*.tmp" -delete || true
find . -name ".DS_Store" -delete || true
find . -name "debug_*" -delete || true
find . -name "tmp_*" -delete || true

echo "Cleanup complete"
