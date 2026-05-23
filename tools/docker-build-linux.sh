#!/usr/bin/env bash
# tools/docker-build-linux.sh
#
# Bygger norscode-bootstrap-compile for Linux x86_64 via Docker.
#
# Krav:
#   - Docker Desktop kjører på macOS (med Rosetta / QEMU for linux/amd64)
#   - build/bootstrap_compiler_bundle_ncb_data.c  finnes
#   - build/native_elf_compiler_bundle_ncb_data.c finnes
#
# Bruk:
#   bash tools/docker-build-linux.sh
#   bash tools/docker-build-linux.sh --output dist/norscode-linux-x86_64
#
# Etter vellykket bygging:
#   dist/norscode-linux-x86_64   (klar til bruk på Linux x86_64)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTPUT="${1:---output}"
if [ "$OUTPUT" = "--output" ]; then
    OUTPUT="${2:-$ROOT/dist/norscode-linux-x86_64}"
fi

BOOTSTRAP_DATA="$ROOT/build/bootstrap_compiler_bundle_ncb_data.c"
NATIVE_DATA="$ROOT/build/native_elf_compiler_bundle_ncb_data.c"
C_VM_DIR="$ROOT/tools/c_minimal_vm"
DOCKERFILE="$ROOT/Dockerfile.linux-build"

# ─── Forutsetningssjekk ──────────────────────────────────────────────────────
_check() {
    if [ ! -f "$1" ]; then
        printf 'Feil: mangler %s\n' "$1" >&2
        printf 'Kjør: python3 main.py selfhost-bootstrap-gate\n' >&2
        exit 1
    fi
}

_check "$BOOTSTRAP_DATA"
_check "$NATIVE_DATA"
_check "$C_VM_DIR/minimal_vm.c"
_check "$DOCKERFILE"

if ! command -v docker >/dev/null 2>&1; then
    printf 'Feil: docker ikke funnet i PATH.\n' >&2
    printf 'Installer Docker Desktop: https://www.docker.com/products/docker-desktop/\n' >&2
    exit 1
fi

if ! docker info >/dev/null 2>&1; then
    printf 'Feil: Docker-daemon kjører ikke.\n' >&2
    printf 'Start Docker Desktop og prøv igjen.\n' >&2
    exit 1
fi

# ─── Bygg ────────────────────────────────────────────────────────────────────
TMPOUT="$(mktemp -d)"
trap 'rm -rf "$TMPOUT"' EXIT

printf 'Bygger norscode-bootstrap-compile for Linux x86_64 via Docker...\n'
printf '  Dockerfile:       %s\n' "$DOCKERFILE"
printf '  Bootstrap-data:   %s (%.0f KB)\n' \
    "$(basename "$BOOTSTRAP_DATA")" "$(( $(wc -c < "$BOOTSTRAP_DATA") / 1024 ))"
printf '  Native-data:      %s (%.0f KB)\n' \
    "$(basename "$NATIVE_DATA")" "$(( $(wc -c < "$NATIVE_DATA") / 1024 ))"
printf '\n'

docker buildx build \
    --platform linux/amd64 \
    --file "$DOCKERFILE" \
    --target export \
    --output "type=local,dest=$TMPOUT" \
    "$ROOT"

# ─── Kopier ut binæren ───────────────────────────────────────────────────────
BUILT="$TMPOUT/norscode-linux-x86_64"
if [ ! -f "$BUILT" ]; then
    printf 'Feil: Docker-bygget fullførte, men binæren ble ikke funnet i output.\n' >&2
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT")"
cp "$BUILT" "$OUTPUT"
chmod +x "$OUTPUT"

SIZE="$(wc -c < "$OUTPUT")"
printf 'Bygget: %s (%d bytes)\n' "$OUTPUT" "$SIZE"

# ─── Sha256-sjekksum ─────────────────────────────────────────────────────────
if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$OUTPUT" > "${OUTPUT}.sha256"
    printf 'Sjekksum: %s\n' "${OUTPUT}.sha256"
elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$OUTPUT" > "${OUTPUT}.sha256"
    printf 'Sjekksum: %s\n' "${OUTPUT}.sha256"
fi

printf '\nLinux x86_64-binary klar.\n'
printf 'Kopier til en Linux-maskin og kjør:\n'
printf '  chmod +x norscode-linux-x86_64\n'
printf '  NORCODE_BOOTSTRAP_VM=1 NORCODE_BOOTSTRAP_CLI=1 \\\n'
printf '    NORCODE_ARGC=1 NORCODE_ARG0=selfcheck ./norscode-linux-x86_64\n'
