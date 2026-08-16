#!/usr/bin/env bash
# regen_from_seed_verify.sh — regenererer omgang6b-fragmenta frå SEED-en si
# kompilering av dei 8 modulane (den trygge bootstrap-kompilatoren), ikkje frå
# den buggy OLD-executoren, og verifiserer Gen1==Gen2 byte-paritet.
# Grunn: char_code(param[idx]) kompilerer korrekt med SEED-en (verifisert), men
# OLD-executoren (juli-fragment) miscompilerte det → fersk-executor crashar.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT="$(pwd)"
SEED="$ROOT/bootstrap/stage0/norscode-linux-x86_64"
[ -x "$SEED" ] || { echo "manglar $SEED"; exit 1; }
mkdir -p dist && cp "$SEED" dist/norscode_native && chmod +x dist/norscode_native
export NORSCODE_ROOT="$ROOT" RUNNER_OS=Linux RUNNER_ARCH=X64 NC_OM6B_RUN_STAGE0=1 NORSCODE_ENABLE_EXEC_PROSESS=1
export NORSCODE_VM_CAPABILITIES=env.read,env.write,process.exec,disk.read,disk.write,net.tcp,net.dns,net.connect
export NORSCODE_VM_DISK_ROOT="$ROOT,.,/tmp,/private/tmp"

git checkout -- bootstrap/precompiled_fragments bootstrap/precompiled_fragments_inner 2>/dev/null || true
SDIR=/tmp/seed_ncb; rm -rf "$SDIR"; mkdir -p "$SDIR"
MARKER="$ROOT/build/6b/selfcompile/stage0_elf_passed.marker"

echo "=== [1] kompiler dei 8 modulane med SEED-en (rett modulnamn) ==="
compile_mod() { # path modname outname
  NORSCODE_VM_CAPABILITIES=env.read,disk.read,disk.write NORSCODE_VM_DISK_ROOT="$ROOT,/tmp" \
  NORSCODE_CMD=compile NORSCODE_FILE="$ROOT/$1" NORSCODE_MODULE="$2" NORSCODE_OUTPUT="$SDIR/$3" "$SEED" >/dev/null 2>&1
  [ -s "$SDIR/$3" ] && echo "  ✓ $3 ($(wc -c <"$SDIR/$3") bytes)" || { echo "  ✗ $2 feila"; return 1; }
}
compile_mod selfhost/lexer/lexer_m1.no        selfhost.lexer.lexer_m1            lexer_m1.ncb.json        || exit 1
compile_mod selfhost/parser.no                selfhost.parser                   parser.ncb.json          || exit 1
compile_mod selfhost/compiler/semantic.no     selfhost.compiler.semantic        semantic.ncb.json        || exit 1
compile_mod selfhost/compiler/ir_to_bytecode.no selfhost.compiler.ir_to_bytecode ir_to_bytecode.ncb.json || exit 1
compile_mod selfhost/json.no                  selfhost.json                     json.ncb.json            || exit 1
compile_mod selfhost/kompiler.no              selfhost.kompiler                 kompiler.ncb.json        || exit 1
compile_mod selfhost/bundler.no               selfhost.bundler                  bundler.ncb.json         || exit 1
compile_mod selfhost/elf_compile_driver.no    selfhost.elf_compile_driver       elf_compile_driver.ncb.json || exit 1

echo "=== [2] regenerer fragment frå SEED-NCB-ane ==="
NORSCODE_FRAGMENT_NCB_DIR="$SDIR" NORSCODE_FRAGMENT_SOURCE_NATIVE=1 \
  NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/regenerate_omgang6b_fragments_safe.no" "$SEED" 2>&1 | tail -10

echo "=== [3] selfcompile → byte-paritet? ==="
rm -rf "$ROOT/build/6b"
NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/selfcompile_stage0_elf.no" "$SEED" > /tmp/sc_seed.log 2>&1
echo "  selfcompile exit=$?"
grep -iE "source-compile|paritet|FEIL] ELF differ|Gen1 ELF [0-9]|Gen2 ELF [0-9]|passert|exit=139" /tmp/sc_seed.log | tail -18
echo ""
if [ -f "$MARKER" ]; then
  echo "🎉🎉 BYTE-PARITET PASSERT — SELF-SUFFICIENCY!"
else
  echo "✗ ikkje passert. Siste 25 linjer av loggen:"; tail -25 /tmp/sc_seed.log
fi
echo "=== fragment-endringar (til commit ved suksess) ==="
git status --short bootstrap/precompiled_fragments bootstrap/precompiled_fragments_inner | head
echo "=== FERDIG — lim heile utskrifta tilbake til Claude ==="
