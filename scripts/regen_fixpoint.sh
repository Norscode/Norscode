#!/usr/bin/env bash
# regen_fixpoint.sh — full self-hosting fixpoint. Bootstrap: SEED-en kompilerer dei
# 8 modulane → fragment (gjev ein WORKING executor). Deretter iterer: regenerer
# fragment frå source_only (executoren si eiga kompilering) til Gen1==Gen2 byte-
# paritet konvergerer. Native Linux x86_64.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT="$(pwd)"
SEED="$ROOT/bootstrap/stage0/norscode-linux-x86_64"
[ -x "$SEED" ] || { echo "manglar $SEED"; exit 1; }
mkdir -p dist && cp "$SEED" dist/norscode_native && chmod +x dist/norscode_native
export NORSCODE_ROOT="$ROOT" RUNNER_OS=Linux RUNNER_ARCH=X64 NC_OM6B_RUN_STAGE0=1 NORSCODE_ENABLE_EXEC_PROSESS=1
export NORSCODE_VM_CAPABILITIES=env.read,env.write,process.exec,disk.read,disk.write,net.tcp,net.dns,net.connect
export NORSCODE_VM_DISK_ROOT="$ROOT,.,/tmp,/private/tmp"
NCBDIR="$ROOT/build/6b/selfcompile/source_only/ncb"
MARKER="$ROOT/build/6b/selfcompile/stage0_elf_passed.marker"
SDIR=/tmp/seed_ncb

selfcompile() { rm -rf "$ROOT/build/6b"; NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/selfcompile_stage0_elf.no" "$SEED" > "$1" 2>&1; }
regen_from() { NORSCODE_FRAGMENT_NCB_DIR="$1" NORSCODE_FRAGMENT_SOURCE_NATIVE=1 \
    NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/regenerate_omgang6b_fragments_safe.no" "$SEED" > /dev/null 2>&1; }
parity() { grep -iE "Gen1 ELF [0-9]|Gen2 ELF [0-9]|FEIL] ELF differ|byte-paritet" "$1" | tail -4; }

git checkout -- bootstrap/precompiled_fragments bootstrap/precompiled_fragments_inner 2>/dev/null || true

echo "=== BOOTSTRAP: SEED kompilerer 8 modular → fragment ==="
rm -rf "$SDIR"; mkdir -p "$SDIR"
cm() { NORSCODE_VM_CAPABILITIES=env.read,disk.read,disk.write NORSCODE_VM_DISK_ROOT="$ROOT,/tmp" \
  NORSCODE_CMD=compile NORSCODE_FILE="$ROOT/$1" NORSCODE_MODULE="$2" NORSCODE_OUTPUT="$SDIR/$3" "$SEED" >/dev/null 2>&1; [ -s "$SDIR/$3" ] || { echo "  ✗ $2"; exit 1; }; }
cm selfhost/lexer/lexer_m1.no selfhost.lexer.lexer_m1 lexer_m1.ncb.json
cm selfhost/parser.no selfhost.parser parser.ncb.json
cm selfhost/compiler/semantic.no selfhost.compiler.semantic semantic.ncb.json
cm selfhost/compiler/ir_to_bytecode.no selfhost.compiler.ir_to_bytecode ir_to_bytecode.ncb.json
cm selfhost/json.no selfhost.json json.ncb.json
cm selfhost/kompiler.no selfhost.kompiler kompiler.ncb.json
cm selfhost/bundler.no selfhost.bundler bundler.ncb.json
cm selfhost/elf_compile_driver.no selfhost.elf_compile_driver elf_compile_driver.ncb.json
regen_from "$SDIR"
echo "  ✓ bootstrap-fragment frå SEED klare"

converged=0
for iter in 1 2 3 4 5; do
  echo "════════ ITERASJON $iter ════════"
  selfcompile /tmp/fx_$iter.log
  echo "  selfcompile exit=$?"; parity /tmp/fx_$iter.log
  if [ -f "$MARKER" ]; then echo "  🎉🎉 BYTE-PARITET PASSERT i iterasjon $iter"; converged=1; break; fi
  if [ ! -f "$NCBDIR/lexer_m1.ncb.json" ]; then
    echo "  [AVBRYT] source-compile feila:"; grep -iE "FEIL|exit=139" /tmp/fx_$iter.log | tail -10; break
  fi
  regen_from "$NCBDIR"
  echo "  → regenererte fragment frå source_only (iterasjon $iter)"
done

echo ""
if [ "$converged" = 1 ]; then
  echo "🎉🎉🎉 SELF-SUFFICIENCY: kompilatoren byggjer seg sjølv byte-identisk!"
else
  echo "✗ ikkje konvergert. Siste iterasjons ELF-storleikar (skal nærme seg kvarandre):"
  ls -la "$ROOT/build/6b/selfcompile/"gen1_compiler.elf "$ROOT/build/6b/selfcompile/"gen2_compiler.elf 2>/dev/null | awk '{print $5,$NF}'
fi
echo "=== fragment-endringar (til commit) ==="
git status --short bootstrap/precompiled_fragments bootstrap/precompiled_fragments_inner | wc -l
echo "=== FERDIG — lim heile utskrifta tilbake ==="
