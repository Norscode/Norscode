#!/usr/bin/env bash

set -e

mkdir -p reports
REPORT="reports/unified_bootstrap_verification.txt"

{
    echo "=== Unified Bootstrap Verification ==="
    echo ""

    echo "Running Python dependency audit..."
    bash tools/python_dependency_audit.sh || true

    echo ""
    echo "Running compiler equivalence verification..."
    if [ -f build/compiler_stage1 ] && [ -f build/compiler_stage2 ]; then
        bash tools/compare_compilers.sh build/compiler_stage1 build/compiler_stage2 || true
    else
        echo "Compiler stage outputs missing"
    fi

    echo ""
    echo "Running bootstrap diff analysis..."
    if [ -f build/compiler_stage1 ] && [ -f build/compiler_stage2 ]; then
        bash tools/bootstrap_diff_analyzer.sh build/compiler_stage1 build/compiler_stage2 || true
    fi

    echo ""
    echo "Running backend ordering validation..."
    if [ -f reports/backend_a.txt ] && [ -f reports/backend_b.txt ]; then
        bash tools/backend_ordering_validator.sh reports/backend_a.txt reports/backend_b.txt || true
    else
        echo "Backend trace artifacts missing"
    fi

    echo ""
    echo "Running semantic diff analysis..."
    if [ -f reports/symbols_a.txt ] && [ -f reports/symbols_b.txt ]; then
        bash tools/semantic_diff_analyzer.sh reports/symbols_a.txt reports/symbols_b.txt || true
    else
        echo "Semantic trace artifacts missing"
    fi

    echo ""
    echo "Collecting bootstrap artifacts..."
    bash tools/bootstrap_artifact_pipeline.sh || true

    echo ""
    echo "Unified bootstrap verification completed"
} > "$REPORT"

echo "Unified bootstrap verification report written to: $REPORT"
