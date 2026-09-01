#!/usr/bin/env sh
# Byggjer ein probe til ELF og køyrer under gdb for å fange krasj-PC + register.
set -eu
PROBE="${1:?bruk: gdb_crash.sh <probe.no>}"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"; cd "$ROOT"
OUT="$ROOT/build/gc-litmus"; mkdir -p "$OUT"
STEM="$(basename "$PROBE" .no)"
CAPS="env.read,env.write,disk.read,disk.write,process.exec"
DR=".,/tmp,/private/tmp,build,selfhost,bootstrap,tests,std,$ROOT"
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_VM_REQUESTED_POLICY=1 \
    NORSCODE_VM_REQUESTED_CAPABILITIES="$CAPS" NORSCODE_VM_CAPABILITIES="$CAPS" \
    NORSCODE_VM_REQUESTED_DISK_ROOT="$DR" NORSCODE_VM_DISK_ROOT="$DR" NORSCODE_ROOT="$ROOT" \
    ./bin/nc bundle --output "$OUT/$STEM.ncb.json" "__main__=$PROBE" >/dev/null
./bin/nc ncb-to-elf "$OUT/$STEM.ncb.json" "$OUT/$STEM.elf"
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
