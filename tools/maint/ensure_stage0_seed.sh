#!/usr/bin/env bash
# tools/maint/ensure_stage0_seed.sh — sørg for bootstrap/stage0/norscode-<plattform>
#
# Normal rekkefølge: committed stage0 → GitHub release
# Maintainer siste utvei: bootstrap/maint/c + clang
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
ENSURE_ROOT="${ROOT}/build/ensure_stage0_seed"

platform_name() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"
    case "$OS" in
        Darwin)
            case "$ARCH" in arm64) printf 'macos-arm64' ;; x86_64) printf 'macos-x86_64' ;; *) return 1 ;; esac ;;
        Linux)
            case "$ARCH" in x86_64|amd64) printf 'linux-x86_64' ;; aarch64|arm64) printf 'linux-arm64' ;; *) return 1 ;; esac ;;
        *) return 1 ;;
    esac
}

platform="$(platform_name)" || {
    printf 'Feil: ukjent plattform\n' >&2
    exit 1
}

DEST="${ROOT}/bootstrap/stage0/norscode-${platform}"
mkdir -p "$(dirname "$DEST")"

if [ -f "$DEST" ]; then
    printf '✓ stage0-seed: %s (%d bytes)\n' "$DEST" "$(wc -c < "$DEST" | tr -d ' ')"
    exit 0
fi

if bash "$ROOT/tools/fetch_stage0_seed.sh" 2>/dev/null; then
    exit 0
fi

printf 'ℹ︎ Maintainer fallback: byggjer mellombels maintainer-output i %s\n' "$ENSURE_ROOT" >&2
rm -rf "$ENSURE_ROOT"
mkdir -p "$ENSURE_ROOT"
if BOOTSTRAP_C_ROOT="$ENSURE_ROOT" NORSCODE_BOOTSTRAP_C=1 REGEN=1 bash "$ROOT/tools/build_norscode_native.sh"; then
    cp "$ROOT/dist/norscode_native" "$DEST"
    chmod +x "$DEST"
    printf '✓ stage0-seed frå isolert maintainer-regen: %s (%d bytes)\n' "$DEST" "$(wc -c < "$DEST" | tr -d ' ')"
    exit 0
fi

printf 'Feil: ingen stage0-seed for %s\n' "$platform" >&2
printf 'Maintainer siste utvei på Linux: køyr isolert regen via BOOTSTRAP_C_ROOT=build/ensure_stage0_seed NORSCODE_BOOTSTRAP_C=1 REGEN=1 bash tools/build_norscode_native.sh og kopier deretter dist/norscode_native til %s\n' "$DEST" >&2
exit 1
