#!/usr/bin/env sh
set -eu
PROBE="${1:?}"; ADDR="${2:-0x600000}"; ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"; cd "$ROOT"
OUT="$ROOT/build/gc-litmus"; mkdir -p "$OUT"; STEM="$(basename "$PROBE" .no)"
CAPS="env.read,env.write,disk.read,disk.write,process.exec"
DR=".,/tmp,/private/tmp,build,selfhost,bootstrap,tests,std,$ROOT"
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_VM_REQUESTED_POLICY=1 \
    NORSCODE_VM_REQUESTED_CAPABILITIES="$CAPS" NORSCODE_VM_CAPABILITIES="$CAPS" \
    NORSCODE_VM_REQUESTED_DISK_ROOT="$DR" NORSCODE_VM_DISK_ROOT="$DR" NORSCODE_ROOT="$ROOT" \
    ./bin/nc compile "$PROBE" "$OUT/$STEM.ncb.json" >/dev/null
env NORSCODE_GC_ALLOC="${NORSCODE_GC_ALLOC:-1}" ./bin/nc ncb-to-elf "$OUT/$STEM.ncb.json" "$OUT/$STEM.elf"
chmod +x "$OUT/$STEM.elf"
which gdb >/dev/null 2>&1 || sudo apt-get install -y gdb >/dev/null 2>&1 || true
echo "=== WATCH $ADDR ==="
gdb -batch -nx \
  -ex "break *0x401000" -ex 'run' \
  -ex "watch *(unsigned long*)$ADDR" \
  -ex 'run' \
  -ex 'printf "WRITE: rip="' -ex 'x/1i $rip' \
  -ex 'c' -ex 'printf "WRITE: rip="' -ex 'x/1i $rip' \
  -ex 'c' -ex 'printf "WRITE: rip="' -ex 'x/1i $rip' \
  -ex 'c' -ex 'printf "WRITE: rip="' -ex 'x/1i $rip' \
  -ex 'c' -ex 'printf "WRITE: rip="' -ex 'x/1i $rip' \
  -ex 'c' -ex 'printf "WRITE: rip="' -ex 'x/1i $rip' \
  "$OUT/$STEM.elf" 2>&1 | grep -E 'WRITE:|0x[0-9a-f]+:|Old value|New value|SIGSEGV' | head -40
