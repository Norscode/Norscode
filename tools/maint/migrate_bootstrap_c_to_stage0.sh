#!/usr/bin/env bash
# tools/maint/migrate_bootstrap_c_to_stage0.sh — maintainer-engangsmigrering til stage0-seed
#
# Dette er ikkje normal bygg/CI-flyt.
# Køyr på Linux x86-64 når lokal maintainer-regen-output finst og du vil lage
# bootstrap/stage0/norscode-linux-x86_64 som ny seed:
#   bash tools/maint/migrate_bootstrap_c_to_stage0.sh
# Commit deretter bootstrap/stage0/norscode-linux-x86_64 og fjern bootstrap/maint/c/norscode_generated.c frå git.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
MIGRATE_ROOT="$ROOT/build/migrate_stage0"

OS="$(uname -s)"
ARCH="$(uname -m)"
if [ "$OS" != "Linux" ] || { [ "$ARCH" != "x86_64" ] && [ "$ARCH" != "amd64" ]; }; then
    printf 'Feil: køyr på Linux x86-64\n' >&2
    exit 1
fi

DEST="$ROOT/bootstrap/stage0/norscode-linux-x86_64"
mkdir -p "$(dirname "$DEST")"
rm -rf "$MIGRATE_ROOT"
mkdir -p "$MIGRATE_ROOT"

printf 'ℹ︎ Maintainer-migrering: byggjer stage0-seed frå isolert regen-output i %s\n' "$MIGRATE_ROOT"
BOOTSTRAP_C_ROOT="$MIGRATE_ROOT" NORSCODE_BOOTSTRAP_C=1 REGEN=1 bash "$ROOT/tools/build_norscode_native.sh"
cp "$ROOT/dist/norscode_native" "$DEST"
chmod +x "$DEST"
printf '✓ %s (%d bytes)\n' "$DEST" "$(wc -c < "$DEST" | tr -d ' ')"
printf 'Neste: git add bootstrap/stage0/norscode-linux-x86_64 && git rm bootstrap/maint/c/norscode_generated.c\n'
