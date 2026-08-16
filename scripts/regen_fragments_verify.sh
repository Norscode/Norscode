#!/usr/bin/env bash
# regen_fragments_verify.sh — bootstrap-fixpoint: regenererer dei stale (16. juli)
# omgang6b-precompiled-fragmenta frå current-source-kompilerte NCB-ar og
# konvergerer Gen1==Gen2 byte-paritet (ELF stage-0 / selvstendighet).
# Native Linux x86_64. Fil-redirect (IKKJE pipe → tail drap selfcompile før).
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT="$(pwd)"
SEED="$ROOT/bootstrap/stage0/norscode-linux-x86_64"
[ -x "$SEED" ] || { echo "manglar $SEED"; exit 1; }
mkdir -p dist && cp "$SEED" dist/norscode_native && chmod +x dist/norscode_native
export NORSCODE_ROOT="$ROOT" RUNNER_OS=Linux RUNNER_ARCH=X64 NC_OM6B_RUN_STAGE0=1 NORSCODE_ENABLE_EXEC_PROSESS=1
export NORSCODE_VM_CAPABILITIES=env.read,env.write,process.exec,disk.read,disk.write,net.tcp,net.dns,net.connect
export NORSCODE_VM_DISK_ROOT="$ROOT,.,/tmp,/private/tmp"
# Nullstill fragmenta til committa (juli) for rein fixpoint-start.
git checkout -- bootstrap/precompiled_fragments bootstrap/precompiled_fragments_inner 2>/dev/null || true
NCBDIR="$ROOT/build/6b/selfcompile/source_only/ncb"
MARKER="$ROOT/build/6b/selfcompile/stage0_elf_passed.marker"

selfcompile() { NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/selfcompile_stage0_elf.no" "$SEED" > "$1" 2>&1; }
regen() { NORSCODE_FRAGMENT_NCB_DIR="$NCBDIR" NORSCODE_FRAGMENT_SOURCE_NATIVE=1 \
    NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/regenerate_omgang6b_fragments_safe.no" "$SEED" 2>&1; }

converged=0
for iter in 1 2 3 4; do
  echo "════════ ITERASJON $iter ════════"
  rm -rf "$ROOT/build/6b"
  selfcompile /tmp/sc_$iter.log; sc_exit=$?
  echo "  selfcompile exit=$sc_exit"
  grep -iE "paritet|FEIL] ELF differ|GROEN|passert|Gen2 ELF [0-9]|Gen1 ELF [0-9]" /tmp/sc_$iter.log | tail -6
  if [ -f "$MARKER" ]; then echo "  ✓✓ BYTE-PARITET PASSERT i iterasjon $iter"; converged=1; break; fi
  if [ ! -f "$NCBDIR/lexer_m1.ncb.json" ]; then
    echo "  [AVBRYT] source_only NCB-ar mangla — source-compile feila:"; grep -iE "FEIL|exit=139|source-compile" /tmp/sc_$iter.log | tail -12; exit 1
  fi
  echo "  --- regenererer fragment frå current-source NCB-ar ---"
  regen | tail -10
done

echo ""
if [ "$converged" = 1 ]; then
  echo "🎉 SELF-SUFFICIENCY: Gen1 ELF == Gen2 ELF byte-identisk"
else
  echo "✗ ikkje konvergert etter 4 iterasjonar (fixpoint-drift). Siste ELF-storleikar:"
  ls -la "$ROOT/build/6b/selfcompile/"gen1_compiler.elf "$ROOT/build/6b/selfcompile/"gen2_compiler.elf 2>/dev/null | awk '{print $5,$NF}'
fi
echo "=== fragment-endringar (til commit) ==="
git status --short bootstrap/precompiled_fragments bootstrap/precompiled_fragments_inner | head
git diff --stat bootstrap/precompiled_fragments_inner | tail -12
echo "=== FERDIG — lim heile utskrifta tilbake til Claude ==="
