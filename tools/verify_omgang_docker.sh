#!/usr/bin/env bash
# verify_omgang_docker.sh — execution-verify Omgang 3-7 native codegen på ekte
# ARM64 Linux via Docker (Apple Silicon køyrer native aarch64).
#
# For KVAR fixtur-NCB i tools/fixtures/ncb_arm64/: bygg ELF via KJELDE-codegen
# (NORSCODE_USE_PRECOMPILED_SELFHOST=0 → ncval_compile_program med omgang-emittarane),
# køyr i Docker linux/arm64, samanlikn exit-koden mot forventa (42, unntak: nc_stype=5).
#
# Krev: Docker Desktop (linux/arm64), bygd bin/nc. Kvar codegen-køyring er ~5-8 min;
# alle ELF byggjast i EIN køyring via tools/elf_multi_host.no.
#
# Dette er stopgap-verifisering FØR seed-rebuild: seed-frontenden parsar ikkje
# M3/M5/M10-syntaks, så fixturane er handlaga NCB-ar. Etter seed-rebuild køyrer
# tools/differensial_sele.no dei ekte .no-fixturane (tools/fixtures/diff_*.no).
set -u
cd "$(dirname "$0")/.."
NCBDIR=tools/fixtures/ncb_arm64
OUT=${OUT:-build/fullhost/debug/verify}
mkdir -p "$OUT"

# forventa exit-kode per fixtur (standard 42)
declare -A EXPECT
for f in "$NCBDIR"/*.ncb.json; do EXPECT[$(basename "$f" .ncb.json)]=42; done

# bygg alle ELF i ein kjelde-codegen-køyring
specs=""
for f in "$NCBDIR"/*.ncb.json; do
  n=$(basename "$f" .ncb.json)
  specs="${specs:+$specs,}$f:$OUT/$n.elf"
done
echo "[verify] byggjer $(ls "$NCBDIR"/*.ncb.json | wc -l | tr -d ' ') ELF via kjelde-codegen (~5-8 min)..."
NC_ELF_SPECS="$specs" NORSCODE_USE_PRECOMPILED_SELFHOST=0 ./bin/nc run tools/elf_multi_host.no

# køyr kvar i Docker linux/arm64
pass=0; fail=0
for f in "$NCBDIR"/*.ncb.json; do
  n=$(basename "$f" .ncb.json)
  elf="$OUT/$n.elf"
  [ -f "$elf" ] || { echo "  ✗ $n: ELF ikkje bygd"; fail=$((fail+1)); continue; }
  chmod +x "$elf"
  docker run --rm --platform linux/arm64 -v "$PWD/$OUT:/w:ro" alpine:3.20 "/w/$n.elf" >/dev/null 2>&1
  code=$?
  exp=${EXPECT[$n]}
  if [ "$code" = "$exp" ]; then echo "  ✓ $n → exit $code"; pass=$((pass+1))
  else echo "  ✗ $n → exit $code (venta $exp)"; fail=$((fail+1)); fi
done
echo "[verify] $pass grøne, $fail raude"
[ "$fail" = 0 ]
