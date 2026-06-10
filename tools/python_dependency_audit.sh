#!/usr/bin/env bash
# tools/python_dependency_audit.sh — tynn launcher for Norscode-basert Python-audit

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OUTPUT_DIR="reports"
OUTPUT_FILE="$OUTPUT_DIR/python_dependency_report.txt"
mkdir -p "$OUTPUT_DIR"

platform_name() {
    local os arch
    os="$(uname -s)"
    arch="$(uname -m)"
    case "$os" in
        Darwin)
            case "$arch" in
                arm64) printf 'macos-arm64' ;;
                x86_64) printf 'macos-x86_64' ;;
                *) return 1 ;;
            esac
            ;;
        Linux)
            case "$arch" in
                x86_64) printf 'linux-x86_64' ;;
                aarch64|arm64) printf 'linux-arm64' ;;
                *) return 1 ;;
            esac
            ;;
        *)
            return 1
            ;;
    esac
}

resolve_native() {
    local platform stage0
    if [ -x "$ROOT/dist/norscode_native" ]; then
        printf '%s\n' "$ROOT/dist/norscode_native"
        return 0
    fi
    if platform="$(platform_name 2>/dev/null)"; then
        stage0="$ROOT/bootstrap/stage0/norscode-$platform"
        if [ -x "$stage0" ]; then
            printf '%s\n' "$stage0"
            return 0
        fi
    fi
    printf 'Feil: fann verken stage-0-seed eller dist/norscode_native for Python-audit\n' >&2
    return 1
}

TOOLS_PY="$(rg --files tools 2>/dev/null | rg '\.py$' || true)"
REPO_PY="$(rg --files -g '*.py' 2>/dev/null || true)"
NC_NATIVE="$(resolve_native)"

env \
    NORSCODE_CMD=run \
    NORSCODE_FILE="$ROOT/scripts/python_dependency_audit.no" \
    NC_PY_AUDIT_TOOLS="$TOOLS_PY" \
    NC_PY_AUDIT_REPO="$REPO_PY" \
    NC_PY_AUDIT_OUTPUT="$OUTPUT_FILE" \
    "$NC_NATIVE"
