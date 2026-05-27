#!/usr/bin/env bash
# tools/docker-build-linux.sh
#
# Legacy helper for packaging the Linux x86_64 bootstrap binary.
# Docker-pipelinen er fjernet; dette skriptet speiler nå den native bootstrap-flyten.
#
# Krav:
#   - dist/norcode-bootstrap-compile finnes eller kan bygges via tools/build-bootstrap-binary.sh
#
# Bruk:
#   bash tools/docker-build-linux.sh
#   bash tools/docker-build-linux.sh --output dist/norscode-linux-x86_64
#
# Etter vellykket bygging:
#   dist/norscode-linux-x86_64   (klar til bruk på Linux x86_64)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ "${1:-}" = "--output" ]; then
    OUTPUT="${2:-$ROOT/dist/norscode-linux-x86_64}"
else
    OUTPUT="${1:-$ROOT/dist/norscode-linux-x86_64}"
fi

BOOTSTRAP_BIN="$ROOT/dist/norcode-bootstrap-compile"

if [ ! -x "$BOOTSTRAP_BIN" ]; then
    printf 'Bootstrap-binæren mangler: %s\n' "$BOOTSTRAP_BIN" >&2
    printf 'Bygg den først med: bash tools/build-bootstrap-binary.sh\n' >&2
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT")"
cp "$BOOTSTRAP_BIN" "$OUTPUT"
chmod +x "$OUTPUT"

SIZE="$(wc -c < "$OUTPUT")"
printf 'Bygget: %s (%d bytes)\n' "$OUTPUT" "$SIZE"

if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$OUTPUT" > "${OUTPUT}.sha256"
    printf 'Sjekksum: %s\n' "${OUTPUT}.sha256"
elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$OUTPUT" > "${OUTPUT}.sha256"
    printf 'Sjekksum: %s\n' "${OUTPUT}.sha256"
fi

printf '\nLinux x86_64-binary klar.\n'
printf 'Kjør for sjekk:\n'
printf '  NORCODE_BOOTSTRAP_VM=1 NORCODE_BOOTSTRAP_CLI=1 \\\n'
printf '    NORCODE_ARGC=1 NORCODE_ARG0=selfcheck %s\n' "$OUTPUT"
