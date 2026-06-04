#!/usr/bin/env bash
# tools/maint/migrate_bootstrap_c_to_stage0.sh — engangsmigrering: C → bootstrap/stage0/ (Omgang 6b.4)
#
# Køyr på Linux x86-64 med clang og bootstrap/maint/c/ til stades:
#   bash tools/maint/migrate_bootstrap_c_to_stage0.sh
# Commit deretter bootstrap/stage0/norscode-linux-x86_64 og git rm bootstrap/maint/c/*.c
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OS="$(uname -s)"
ARCH="$(uname -m)"
if [ "$OS" != "Linux" ] || { [ "$ARCH" != "x86_64" ] && [ "$ARCH" != "amd64" ]; }; then
    printf 'Feil: køyr på Linux x86-64\n' >&2
    exit 1
fi

DEST="$ROOT/bootstrap/stage0/norscode-linux-x86_64"
mkdir -p "$(dirname "$DEST")"

NORSCODE_BOOTSTRAP_C=1 bash "$ROOT/tools/build_norscode_native.sh"
cp "$ROOT/dist/norscode_native" "$DEST"
chmod +x "$DEST"
printf '✓ %s (%d bytes)\n' "$DEST" "$(wc -c < "$DEST" | tr -d ' ')"
printf 'Neste: git add bootstrap/stage0/norscode-linux-x86_64 && git rm bootstrap/maint/c/norscode_generated.c bootstrap/maint/c/nc_dispatch.c\n'
