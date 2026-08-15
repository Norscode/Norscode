#!/usr/bin/env bash
# gen1_gdb_diagnose.sh — køyr på ein NATIV Linux x86_64-host (t.d. Gigahost-VPS).
# Reproduserer gen1-source-executor-segfaulten og fangar crash-PC + disassembly
# så codegen-bugen kan lokaliserast. Emulert Docker/qemu blokkerer ptrace, difor
# må dette køyrast native. Lim heile utskrifta tilbake til Claude.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT="$(pwd)"
SEED="$ROOT/bootstrap/stage0/norscode-linux-x86_64"

command -v gdb >/dev/null || { echo "INSTALLER gdb: sudo apt-get install -y gdb"; exit 1; }
[ -x "$SEED" ] || { echo "manglar $SEED"; exit 1; }
file "$SEED"; echo

export NORSCODE_ROOT="$ROOT" RUNNER_OS=Linux RUNNER_ARCH=X64 NC_OM6B_RUN_STAGE0=1 NORSCODE_ENABLE_EXEC_PROSESS=1
export NORSCODE_VM_CAPABILITIES=env.read,env.write,process.exec,disk.read,disk.write,net.tcp,net.dns,net.connect
export NORSCODE_VM_DISK_ROOT="$ROOT,.,/tmp,/private/tmp"
cp "$SEED" dist/norscode_native 2>/dev/null || true

echo "=== [1] byggjer gen1 ELF-ane (native_codegen_v2 frå committa NCB) ==="
NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/selfcompile_stage0_elf.no" "$SEED" 2>&1 | sed -n '1,40p'
EXE="$ROOT/build/6b/selfcompile/gen1_source_executor.elf"
[ -x "$EXE" ] || { echo "gen1_source_executor.elf blei ikkje bygd — lim utskrifta over"; exit 1; }
echo; echo "=== gen1_source_executor.elf ==="; ls -la "$EXE"; file "$EXE"; echo

# Minimal input som crashar (triviell modul crashar òg → ikkje input-spesifikt).
printf 'funksjon start() -> heiltall { returner 5 }\n' > /tmp/gen1_tiny.no
export NORSCODE_GEN1_SOURCE_FILE=/tmp/gen1_tiny.no
export NORSCODE_GEN1_MODULE=__main__
export NORSCODE_GEN1_OUTPUT_FILE=/tmp/gen1_tiny_out.ncb.json
export NORSCODE_GEN1_PROGRESS_FILE=/tmp/gen1_prog.txt
export NORSCODE_VM_FAST=1
rm -f /tmp/gen1_prog.txt

echo "=== [2] direkte køyr (forvent Segmentation fault / exit 139) ==="
"$EXE"; echo "EXIT=$?"
echo "siste safe-point: $(cat /tmp/gen1_prog.txt 2>/dev/null)"; echo

echo "=== [3] gdb: crash-PC, register, faulting instruksjon, backtrace ==="
gdb -q -batch \
  -ex 'set pagination off' \
  -ex 'run' \
  -ex 'printf "\n--- SIGNAL/PC ---\n"' \
  -ex 'info registers rip rsp rbp rax rbx rcx rdx rsi rdi r8 r9 r10 r11 r12 r13 r14 r15' \
  -ex 'printf "\n--- FAULTING INSN (16 rundt rip) ---\n"' \
  -ex 'x/12i $rip-24' \
  -ex 'printf "\n--- MEM @ rip ---\n"' \
  -ex 'x/32xb $rip-16' \
  -ex 'printf "\n--- BACKTRACE ---\n"' \
  -ex 'bt' \
  -ex 'printf "\n--- STACK (topp) ---\n"' \
  -ex 'x/16xg $rsp' \
  "$EXE" 2>&1 | sed -n '1,80p'

echo; echo "=== [4] ELF .text base (for å mappe rip → offset i codegen-imaget) ==="
readelf -lS "$EXE" 2>/dev/null | grep -iE 'LOAD|\.text|Entry' | head
echo "=== FERDIG — lim heile utskrifta tilbake til Claude ==="
