#!/usr/bin/env bash
# regen_fragments_verify.sh — bootstrap-fixpoint: regenererer dei committa
# omgang6b-precompiled-fragmenta frå current source og verifiserer Gen1==Gen2
# byte-paritet (ELF stage-0 / selvstendighet-porten). Køyr på NATIV Linux x86_64.
#
# Dei committa fragmenta (bootstrap/precompiled_fragments_inner/, frå 16. juli)
# er stale mot current source → [4/4] byte-paritet feilar (Gen1 2.6MB != Gen2
# 0.9MB). Dette regenererer dei frå source-only-NCB-ane som Gen1 produserer.
# Codegen er deterministisk, så éin iterasjon bør konvergere; skriptet køyrer to
# for å stadfeste fixpoint.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT="$(pwd)"
SEED="$ROOT/bootstrap/stage0/norscode-linux-x86_64"
[ -x "$SEED" ] || { echo "manglar $SEED"; exit 1; }
mkdir -p dist && cp "$SEED" dist/norscode_native && chmod +x dist/norscode_native
export NORSCODE_ROOT="$ROOT" RUNNER_OS=Linux RUNNER_ARCH=X64 NC_OM6B_RUN_STAGE0=1 NORSCODE_ENABLE_EXEC_PROSESS=1
export NORSCODE_VM_CAPABILITIES=env.read,env.write,process.exec,disk.read,disk.write,net.tcp,net.dns,net.connect
export NORSCODE_VM_DISK_ROOT="$ROOT,.,/tmp,/private/tmp"

selfcompile() { NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/selfcompile_stage0_elf.no" "$SEED" 2>&1; }
NCBDIR="$ROOT/build/6b/selfcompile/source_only/ncb"

for iter in 1 2; do
  echo "════════ ITERASJON $iter ════════"
  echo "=== [$iter.1] selfcompile → source_only NCBs + paritet-sjekk ==="
  selfcompile | grep -iE "source-compile|paritet|FEIL|GROEN|passert|differ|Stage-0 ELF|✓ Omgang|✗|DELVIS" | tail -18
  [ -f "$NCBDIR/lexer_m1.ncb.json" ] || { echo "[$iter] manglar source_only NCBs — sjå heile utskrifta"; selfcompile | tail -30; exit 1; }
  echo "=== [$iter.2] regenerer fragment frå source_only NCBs ==="
  NORSCODE_FRAGMENT_NCB_DIR="$NCBDIR" NORSCODE_FRAGMENT_SOURCE_NATIVE=1 \
    NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/regenerate_omgang6b_fragments_safe.no" "$SEED" 2>&1 | tail -12
  echo "=== [$iter.3] fragment-endringar ==="
  git status --short bootstrap/precompiled_fragments bootstrap/precompiled_fragments_inner | head
  rm -rf "$ROOT/build/6b"
done

echo "════════ FINAL byte-paritet-sjekk (ferske fragment) ════════"
selfcompile | grep -iE "paritet|FEIL|GROEN|passert|differ|Stage-0 ELF|✓ Omgang|✗|marker|DELVIS" | tail -12
echo ""
echo "=== oppsummering: passerte porten? ==="
if [ -f "$ROOT/build/6b/selfcompile/stage0_elf_passed.marker" ]; then
  echo "✓✓ BYTE-PARITET PASSERT — commit fragment-endringane"
else
  echo "✗ ikkje konvergert endå (sjå over). Lim utskrifta tilbake til Claude."
fi
echo "=== git diff-stat av fragment (til commit) ==="
git status --short bootstrap/precompiled_fragments bootstrap/precompiled_fragments_inner | head
echo "=== FERDIG — lim heile utskrifta tilbake til Claude ==="
