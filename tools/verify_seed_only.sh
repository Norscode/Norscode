#!/usr/bin/env bash
# tools/verify_seed_only.sh — verifiser seed-first utan clang/regen
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

platform_name() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"
    case "$OS" in
        Darwin)
            case "$ARCH" in arm64) printf 'macos-arm64' ;; x86_64) printf 'macos-x86_64' ;; *) return 1 ;; esac ;;
        Linux)
            case "$ARCH" in x86_64) printf 'linux-x86_64' ;; aarch64|arm64) printf 'linux-arm64' ;; *) return 1 ;; esac ;;
        *) return 1 ;;
    esac
}

platform="$(platform_name 2>/dev/null || printf '?')"
stage0="${ROOT}/bootstrap/stage0/norscode-${platform}"
if [ ! -f "$stage0" ]; then
    printf '=== Seed-only: hoppa over (ingen stage0-seed for %s) ===\n' "$platform"
    printf 'Seed-only lane krev bootstrap/stage0/norscode-<plattform> (eller bash tools/fetch_stage0_seed.sh).\n'
    exit 0
fi

printf '=== Seed-only verifikasjon (utan clang/regen) ===\n'
printf '1. Bygg frå seed med ugyldig CC (skal framleis passere)\n'
CC=__clang_not_allowed__ REGEN=0 bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

printf '2. Køyr native testløp\n'
sh "$ROOT/tools/nc_test.sh"
printf '\n'

printf '=== Seed-only verifikasjon: BESTÅTT ===\n'
