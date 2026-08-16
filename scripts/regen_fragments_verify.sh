#!/usr/bin/env bash
# regen_fragments_verify.sh — steg 1: DIAGNOSE. Køyrer selfcompile ÉIN gong, viser
# heile [2/4]+[4/4]-utfallet og kvar source-only-modul-NCB-ane faktisk ligg, så
# fragment-regenereringa kan drivast korrekt. Native Linux x86_64.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT="$(pwd)"
SEED="$ROOT/bootstrap/stage0/norscode-linux-x86_64"
[ -x "$SEED" ] || { echo "manglar $SEED"; exit 1; }
mkdir -p dist && cp "$SEED" dist/norscode_native && chmod +x dist/norscode_native
export NORSCODE_ROOT="$ROOT" RUNNER_OS=Linux RUNNER_ARCH=X64 NC_OM6B_RUN_STAGE0=1 NORSCODE_ENABLE_EXEC_PROSESS=1
export NORSCODE_VM_CAPABILITIES=env.read,env.write,process.exec,disk.read,disk.write,net.tcp,net.dns,net.connect
export NORSCODE_VM_DISK_ROOT="$ROOT,.,/tmp,/private/tmp"

rm -rf "$ROOT/build/6b"
echo "=== selfcompile-stage0-elf (éin køyring, full logg) ==="
NORSCODE_CMD=run NORSCODE_FILE="$ROOT/tools/selfcompile_stage0_elf.no" "$SEED" > /tmp/sc.log 2>&1
echo "selfcompile exit=$?"
echo "--- [2/4] source-compile + [4/4] paritet (frå loggen) ---"
grep -iE "source-compile|Gen2|paritet|byte-iden|FEIL|GROEN|passert|differ|Stage-0 ELF|DELVIS|✓ Omgang|✗|exit=139" /tmp/sc.log | tail -30
echo ""
echo "=== kvar ligg source-only modul-NCB-ane? ==="
find "$ROOT/build/6b" -name "*.ncb.json" 2>/dev/null | grep -iE "source_only|lexer_m1|parser|semantic|ir_to_bytecode|kompiler|bundler|json|elf_compile" | grep -v "executor" | head -20
echo "--- storleikar (source_only/ncb) ---"
ls -la "$ROOT/build/6b/selfcompile/source_only/ncb/"*.ncb.json 2>/dev/null | awk '{print $5, $NF}'
echo ""
echo "=== gen1 vs gen2 ELF-storleik (paritet-målet) ==="
ls -la "$ROOT/build/6b/selfcompile/gen1_compiler.elf" "$ROOT/build/6b/selfcompile/gen2_compiler.elf" 2>/dev/null | awk '{print $5, $NF}'
echo "=== FERDIG — lim heile utskrifta tilbake til Claude ==="
