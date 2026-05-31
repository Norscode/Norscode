#!/usr/bin/env bash
# tools/test_kernel_boot.sh — QEMU-boottest for Norscode OS-kernel
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Finn tilgjengeleg QEMU
QEMU=""
for q in qemu-system-x86_64 qemu-system-i386; do
    if command -v "$q" >/dev/null 2>&1; then
        QEMU="$q"
        break
    fi
done

if [ -z "$QEMU" ]; then
    printf "HOPP: ingen QEMU funnen\n"
    exit 0
fi

printf "Brukar: $QEMU\n"

# Test kernel v1 (32-bit, Multiboot1, VGA "Norscode OS")
KERNEL="$ROOT/build/norscode_kernel.elf"
NC_OUTPUT="$KERNEL" "$ROOT/bin/nc" run \
    "$ROOT/selfhost/native_execution/kernel_builder.no" >/dev/null

VGA=$(
  (
    printf "cont\n"
    sleep 3
    printf "xp/32xb 0xb8000\n"
    sleep 0.3
    printf "quit\n"
  ) | "$QEMU" \
        -kernel "$KERNEL" \
        -display none \
        -S \
        -monitor stdio 2>&1
)

if echo "$VGA" | grep -q "0x4e 0x0e 0x6f 0x0e 0x72 0x0e"; then
    printf "✓ Kernel v1: 'Nor...' i VGA (Multiboot1 OK)\n"
else
    printf "✗ FEIL: kernel v1 VGA-innhald ikkje som forventa\n"
    exit 1
fi

printf "✓ Norscode OS bootar korrekt\n"
