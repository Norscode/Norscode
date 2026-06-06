#!/usr/bin/env bash
# tools/maint/ensure_stage0_seed.sh — sørg for bootstrap/stage0/norscode-<plattform>
#
# Rekkefølge: committed stage0 → GitHub release → (siste utvei) bootstrap/maint/c + clang
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

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

if [ -f "${ROOT}/bootstrap/maint/c/norscode_generated.c" ] \
    && [ -f "${ROOT}/bootstrap/maint/c/nc_dispatch.c" ]; then
    printf 'ℹ︎ Migrerer seed frå bootstrap/maint/c/ (ein gong — fjern C frå git etterpå)\n' >&2
    NORSCODE_BOOTSTRAP_C=1 bash "$ROOT/tools/build_norscode_native.sh"
    cp "$ROOT/dist/norscode_native" "$DEST"
    chmod +x "$DEST"
    printf '✓ stage0-seed frå bootstrap/maint/c: %s (%d bytes)\n' "$DEST" "$(wc -c < "$DEST" | tr -d ' ')"
    exit 0
fi

printf 'Feil: ingen stage0-seed for %s\n' "$platform" >&2
printf 'Køyr på Linux: NORSCODE_BOOTSTRAP_C=1 bash tools/build_norscode_native.sh && cp dist/norscode_native %s\n' "$DEST" >&2
exit 1
