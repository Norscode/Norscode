#!/usr/bin/env sh
# Generisk GC-probe-køyrar (Fase 3): byggjer ein probe-.no til self-hosted native
# ELF via native_codegen_v2 (ncb-to-elf) og køyrer han. Exit 0 = proben verkar.
# Bruk: sh archive/legacy_shell/tools/gc_probe_run.sh <probe.no> [label]
# Kvar GC-fase (F1 primitiv, F2 obj_addr/layout, …) får sin probe og blir eit
# GATING steg i gc-litmus.yml — validerer GC-codegen mot ekte native maskinkode.
set -eu

PROBE="${1:?bruk: gc_probe_run.sh <probe.no> [label]}"
LABEL="${2:-GC-probe}"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
cd "$ROOT"
OUT="$ROOT/build/gc-litmus"
mkdir -p "$OUT"
STEM="$(basename "$PROBE" .no)"

CAPS="env.read,env.write,disk.read,disk.write,process.exec"
DR=".,/tmp,/private/tmp,build,selfhost,bootstrap,tests,std,$ROOT"

# VIKTIG: bruk `nc compile` (seeden sin GODE innebygde compiler), IKKJE `nc bundle`
# (som lastar ein STALE selfhost-compiler-modul som DROPPAR list-literal-bytecode →
# null-lister → krasj). `nc compile` løyser importar OG kompilerer BUILD_LIST/INDEX_GET rett.
echo "=== [$LABEL] 1) compile $PROBE → NCB (nc compile, ikkje bundle) ==="
env NORSCODE_ENABLE_EXEC_PROSESS=1 \
    NORSCODE_VM_REQUESTED_POLICY=1 \
    NORSCODE_VM_REQUESTED_CAPABILITIES="$CAPS" NORSCODE_VM_CAPABILITIES="$CAPS" \
    NORSCODE_VM_REQUESTED_DISK_ROOT="$DR" NORSCODE_VM_DISK_ROOT="$DR" \
    NORSCODE_ROOT="$ROOT" \
    ./bin/nc compile "$PROBE" "$OUT/$STEM.ncb.json" >/dev/null

echo "=== [$LABEL] 2) ncb-to-elf (self-hosted native_codegen_v2) ==="
env NORSCODE_NCB_TO_ELF_INPUT="$OUT/$STEM.ncb.json" NORSCODE_NCB_TO_ELF_OUTPUT="$OUT/$STEM.elf" NORSCODE_ENABLE_EXEC_PROSESS=1 ./bin/nc run tools/ncb_to_elf.no
chmod +x "$OUT/$STEM.elf"

echo "=== [$LABEL] 3) køyr probe ==="
set +e
"$OUT/$STEM.elf"
rc=$?
set -e
echo "$STEM exit=$rc"
if [ "$rc" -eq 0 ]; then
    echo "[GRØN] $LABEL verkar."
    exit 0
fi
echo "[FEIL] $LABEL feila (exit=$rc)."
exit "$rc"
