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
    if [ "${VERIFY_SEED_ALLOW_MISSING:-0}" = "1" ]; then
        printf '=== Seed-only: hoppa over (ingen stage0-seed for %s) ===\n' "$platform"
        printf 'Seed-only lane krev bootstrap/stage0/norscode-<plattform> (eller bash tools/fetch_stage0_seed.sh).\n'
        exit 0
    fi
    printf '=== Seed-only: FEIL (ingen stage0-seed for %s) ===\n' "$platform" >&2
    printf 'Seed-only lane krev bootstrap/stage0/norscode-<plattform> (eller bash tools/fetch_stage0_seed.sh).\n'
    exit 1
fi

printf '=== Seed-only verifikasjon (utan clang/regen) ===\n'
printf '1. Bygg frå seed med ugyldig CC (skal framleis passere)\n'
CC=__clang_not_allowed__ REGEN=0 bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

printf '2. Køyr seed-smoke\n'
tmp_no="$(mktemp "${TMPDIR:-/tmp}/nc_seed_only_XXXXXX.no" 2>/dev/null || echo "${TMPDIR:-/tmp}/nc_seed_only_$$.no")"
trap 'rm -f "$tmp_no"' EXIT
printf 'funksjon start() { returner 0 }\n' > "$tmp_no"
NORSCODE_CMD=run NORSCODE_FILE="$tmp_no" "$ROOT/dist/norscode_native" >/dev/null
printf '  [OK] seed-smoke\n'
rm -f "$tmp_no"
trap - EXIT
printf '\n'

printf '=== Seed-only verifikasjon: BESTÅTT ===\n'
