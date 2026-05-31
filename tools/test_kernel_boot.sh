#!/usr/bin/env bash
# tools/test_kernel_boot.sh — QEMU-boottest for Norscode OS-kernel
#
# Bygger kernelen, bootar i QEMU, les VGA-minnet og verifiserer output.
# Krev: qemu-system-i386
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
KERNEL="$ROOT/build/norscode_kernel.elf"

if ! command -v qemu-system-i386 >/dev/null 2>&1; then
    printf "HOPP: qemu-system-i386 ikkje funnen\n"
    exit 0
fi

# Bygg kernel
printf "Bygger norscode_kernel.elf...\n"
NC_OUTPUT="$KERNEL" "$ROOT/bin/nc" run \
    "$ROOT/selfhost/native_execution/kernel_builder.no" >/dev/null

# Køyr QEMU og les VGA
VGA=$(
  (
    printf "cont\n"
    sleep 3
    printf "xp/32xb 0xb8000\n"
    sleep 0.3
    printf "quit\n"
  ) | qemu-system-i386 \
        -kernel "$KERNEL" \
        -display none \
        -S \
        -monitor stdio 2>&1
)

# Sjekk at "N" (0x4e) er første VGA-teikn
if echo "$VGA" | grep -q "0x4e 0x0e 0x6f 0x0e 0x72 0x0e"; then
    printf "✓ QEMU-boot: 'Nor...' funnen i VGA-minne (0xB8000)\n"
    printf "✓ Norscode OS bootar korrekt\n"
else
    printf "✗ FEIL: VGA-minne inneheldt ikkje forventa data\n"
    echo "$VGA" | grep "000b8000" | head -3
    exit 1
fi
