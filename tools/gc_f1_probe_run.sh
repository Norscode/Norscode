#!/usr/bin/env sh
# GC F1-probe: byggjer og køyrer råminne-primitiv-proben (raw_store64/raw_load64/
# heap_bump) som ein self-hosted native ELF. Exit 0 = F1-primitiva verkar (store→
# load round-trip stemmer). GATING: F1 er fundamentet for GC-en og MÅ vere grønt.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
OUT="$ROOT/build/gc-litmus"
mkdir -p "$OUT"

CAPS="env.read,env.write,disk.read,disk.write,process.exec"
DR=".,/tmp,/private/tmp,build,selfhost,bootstrap,tests,std,$ROOT"

echo "=== 1) bundle F1-probe → NCB ==="
env NORSCODE_ENABLE_EXEC_PROSESS=1 \
    NORSCODE_VM_REQUESTED_POLICY=1 \
    NORSCODE_VM_REQUESTED_CAPABILITIES="$CAPS" NORSCODE_VM_CAPABILITIES="$CAPS" \
    NORSCODE_VM_REQUESTED_DISK_ROOT="$DR" NORSCODE_VM_DISK_ROOT="$DR" \
    NORSCODE_ROOT="$ROOT" \
    ./bin/nc bundle --output "$OUT/f1probe.ncb.json" \
        "__main__=tests/fixtures/gc_f1_probe.no" >/dev/null

echo "=== 2) ncb-to-elf (self-hosted native_codegen_v2 med F1-primitiva) ==="
./bin/nc ncb-to-elf "$OUT/f1probe.ncb.json" "$OUT/f1probe.elf"
chmod +x "$OUT/f1probe.elf"

echo "=== 3) køyr F1-probe ==="
set +e
"$OUT/f1probe.elf"
rc=$?
set -e
echo "f1probe exit=$rc"
if [ "$rc" -eq 0 ]; then
    echo "[GRØN] F1-primitiva (raw_load64/raw_store64/heap_bump) verkar."
    exit 0
fi
echo "[FEIL] F1-primitiv-proben feila (exit=$rc) — GC-fundamentet er ikkje trygt."
exit "$rc"
