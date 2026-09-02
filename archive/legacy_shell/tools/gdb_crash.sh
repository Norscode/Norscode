#!/usr/bin/env sh
# Byggjer ein probe til ELF og køyrer under gdb for å fange krasj-PC + register.
set -eu
PROBE="${1:?bruk: gdb_crash.sh <probe.no>}"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"; cd "$ROOT"
OUT="$ROOT/build/gc-litmus"; mkdir -p "$OUT"
STEM="$(basename "$PROBE" .no)"
CAPS="env.read,env.write,disk.read,disk.write,process.exec"
DR=".,/tmp,/private/tmp,build,selfhost,bootstrap,tests,std,$ROOT"
NATIVE="${NORSCODE_NATIVE_BIN:-$ROOT/dist/norscode_native}"  # driv native direkte via miljø (seed-uavhengig)
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_VM_REQUESTED_POLICY=1 \
    NORSCODE_VM_REQUESTED_CAPABILITIES="$CAPS" NORSCODE_VM_CAPABILITIES="$CAPS" \
    NORSCODE_VM_REQUESTED_DISK_ROOT="$DR" NORSCODE_VM_DISK_ROOT="$DR" NORSCODE_ROOT="$ROOT" \
    NORSCODE_CMD=compile NORSCODE_FILE="$ROOT/$PROBE" NORSCODE_OUTPUT="$OUT/$STEM.ncb.json" \
    "$NATIVE" >/dev/null
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_VM_CAPABILITIES="$CAPS" NORSCODE_VM_DISK_ROOT="$DR" NORSCODE_ROOT="$ROOT" NORSCODE_NATIVE_BIN="$NATIVE" NORSCODE_NCB_TO_ELF_INPUT="$OUT/$STEM.ncb.json" NORSCODE_NCB_TO_ELF_OUTPUT="$OUT/$STEM.elf" NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/ncb_to_elf.no" "$NATIVE"
chmod +x "$OUT/$STEM.elf"
which gdb >/dev/null 2>&1 || sudo apt-get install -y gdb >/dev/null 2>&1 || true
echo "=== GDB crash analyse ==="
gdb -batch -nx \
  -ex 'run' \
  -ex 'echo \n=== CRASH PC + REGS ===\n' \
  -ex 'info registers rip rax rbx rcx rdx rsi rdi r8 r9 r10 r11 rsp' \
  -ex 'x/6i $rip-8' \
  -ex 'bt' \
  "$OUT/$STEM.elf" 2>&1 | grep -vE '^\[|warning:|No such|during startup' | head -40
