#!/usr/bin/env sh
# Fase 3 GC-litmus: bygg 10k-hmac self-hosted native ELF (native_codegen_v2 sin
# AOT-runtime) og køyr han. Bump-allokator utan reclaim → SIGSEGV (exit 139) =
# den RAUDE baselinen / veggen. Exit 0 = GC-en verkar (Fase 3-målet).
#
# Køyrer på Linux x86_64 (ELF). ncb-to-elf byggjer sjølve ELF-en frå eit lite
# bundla program — lett å byggje; tungt berre å KØYRE (garbage-rate ~65 KB/iter).
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
OUT="$ROOT/build/gc-litmus"
mkdir -p "$OUT"

CAPS="env.read,env.write,disk.read,disk.write,process.exec"
DR=".,/tmp,/private/tmp,build,selfhost,bootstrap,tests,std,$ROOT"

# VIKTIG: `nc compile` (seeden sin GODE compiler) løyser std.sha256-importen OG kompilerer
# list-literal-bytecode (BUILD_LIST/INDEX_GET) rett. `nc bundle` brukte ein stale selfhost-
# compiler som DROPPA list-literalane → null key/msg → krasj på fyrste hmac (feildiagnostisert
# som «heap-exhaustion»). Med nc compile køyrer litmusen faktisk → testar GC-en på ekte.
echo "=== 1) compile litmus (+ std.sha256, importar løyst) → NCB ==="
env NORSCODE_ENABLE_EXEC_PROSESS=1 \
    NORSCODE_VM_REQUESTED_POLICY=1 \
    NORSCODE_VM_REQUESTED_CAPABILITIES="$CAPS" NORSCODE_VM_CAPABILITIES="$CAPS" \
    NORSCODE_VM_REQUESTED_DISK_ROOT="$DR" NORSCODE_VM_DISK_ROOT="$DR" \
    NORSCODE_ROOT="$ROOT" \
    ./bin/nc compile tests/fixtures/gc_litmus_hmac.no "$OUT/litmus.ncb.json" >/dev/null

echo "=== 2) ncb-to-elf (self-hosted native_codegen_v2 AOT-runtime) ==="
./bin/nc ncb-to-elf "$OUT/litmus.ncb.json" "$OUT/litmus.elf"
chmod +x "$OUT/litmus.elf"

echo "=== 3) køyr GC-litmus (10000 hmac_sha256_bytes) ==="
set +e
"$OUT/litmus.elf"
rc=$?
set -e
echo "litmus exit=$rc"

if [ "$rc" -eq 0 ]; then
    echo "[GRØN] GC-litmus BESTÅTT — AOT-heapen toler 10k hmac. Fase 3-GC verkar."
    exit 0
fi
echo "[RAUD BASELINE] GC-litmus KRASJA (exit=$rc; SIGSEGV=139 venta) — bump-"
echo "allokator utan GC sprenger 2 GB-heapen. Dette ER Fase 3-veggen; målet er"
echo "å gjere denne litmusen grøn med hand-emittert mark-sweep-GC (AOT_GC_DESIGN)."
exit "$rc"
