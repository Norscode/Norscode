#!/usr/bin/env bash
# tools/selfcompile_stage0_elf.sh — Omgang 6b.3: Gen1 stage-0 ELF → Gen2 ELF
#
# Gen1: host byggjer stage-0 NCB + ELF.
# Gen2: krev ekte preset-bundle via elf_compile_driver.
# Full stage-0 bestått krev at Gen1 ELF sjølv skriv Gen2 NCB.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# shellcheck source=omgang6b_compiler_bundle_args.inc.sh
. "$ROOT/tools/omgang6b_compiler_bundle_args.inc.sh"

OS="$(uname -s)"
ARCH="$(uname -m)"
CHUNK_SIZE="${NC_OM6B_CHUNK_SIZE:-65536}"

GEN1_NCB="$ROOT/build/6b/compiler_stage0.ncb.json"
GEN1_ELF="$ROOT/build/6b/selfcompile/gen1_compiler.elf"
GEN2_NCB="$ROOT/build/6b/selfcompile/gen2.ncb.json"
GEN2_ELF="$ROOT/build/6b/selfcompile/gen2_compiler.elf"
GEN1_NCB_CHUNK_DIR="$ROOT/build/6b/selfcompile/gen1_ncb_chunks"
GEN1_ELF_LOG="$ROOT/build/6b/selfcompile/gen1_elf_bundle.log"
GEN1_ELF_PRESET_LOG="$ROOT/build/6b/selfcompile/gen1_elf_preset.log"
GEN1_ELF_SOURCE_LOG="$ROOT/build/6b/selfcompile/gen1_elf_source_ncb.log"
GEN1_ELF_CHUNK_LOG="$ROOT/build/6b/selfcompile/gen1_elf_chunked_source_ncb.log"
GEN1_ELF_DIAG="$ROOT/build/6b/selfcompile/gen1_elf_diagnose.txt"
GEN1_ELF_ATTEMPTS="$ROOT/build/6b/selfcompile/gen1_elf_attempts.txt"
PASS_MARKER="$ROOT/build/6b/selfcompile/stage0_elf_passed.marker"

mkdir -p "$ROOT/build/6b/selfcompile"
rm -f "$PASS_MARKER"
rm -f "$GEN1_ELF_LOG"
rm -f "$GEN1_ELF_PRESET_LOG"
rm -f "$GEN1_ELF_SOURCE_LOG"
rm -f "$GEN1_ELF_CHUNK_LOG"
rm -f "$GEN1_ELF_DIAG"
rm -f "$GEN1_ELF_ATTEMPTS"

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

lag_gen1_ncb_chunks() {
    local src="$1"
    local out_dir="$2"
    local chunk_i
    rm -rf "$out_dir"
    mkdir -p "$out_dir"
    split -b "$CHUNK_SIZE" -d -a 3 "$src" "$out_dir/part_"
    chunk_i=0
    for chunk in "$out_dir"/part_*; do
        mv "$chunk" "$out_dir/part_$(printf '%03d' "$chunk_i").json"
        chunk_i=$((chunk_i + 1))
    done
    if [ "$chunk_i" -eq 0 ]; then
        printf '  [FEIL] Ingen chunks laga frå %s\n' "$src" >&2
        exit 1
    fi
    printf '%s' "$chunk_i"
}

skriv_gen1_elf_diagnose() {
    local modus="$1"
    local rc="$2"
    local cmd_txt="${3:-}"
    local log_path="${4:-$GEN1_ELF_LOG}"
    {
        printf 'modus=%s\n' "$modus"
        printf 'exit_code=%s\n' "$rc"
        printf 'os=%s\n' "$OS"
        printf 'arch=%s\n' "$ARCH"
        printf 'gen1_elf=%s\n' "$GEN1_ELF"
        if [ -n "$cmd_txt" ]; then
            printf 'cmd=%s\n' "$cmd_txt"
        fi
        printf '\n[miljo]\n'
        printf 'NC_OM6B_RUN_STAGE0=%s\n' "${NC_OM6B_RUN_STAGE0:-}"
        printf 'NORSCODE_OM6B_PRESET=%s\n' "${NORSCODE_OM6B_PRESET:-}"
        printf 'NORSCODE_OM6B_SOURCE_NCB=%s\n' "${NORSCODE_OM6B_SOURCE_NCB:-}"
        printf 'NORSCODE_OM6B_SOURCE_NCB_DIR=%s\n' "${NORSCODE_OM6B_SOURCE_NCB_DIR:-}"
        printf 'NORSCODE_OM6B_SOURCE_NCB_COUNT=%s\n' "${NORSCODE_OM6B_SOURCE_NCB_COUNT:-}"
        printf 'NORSCODE_BUNDLE_OUTPUT=%s\n' "${NORSCODE_BUNDLE_OUTPUT:-}"
        printf 'NORSCODE_BUNDLE_ENTRY=%s\n' "${NORSCODE_BUNDLE_ENTRY:-}"
        if [ -f "$GEN1_ELF" ]; then
            printf 'gen1_elf_bytes=%s\n' "$(wc -c < "$GEN1_ELF" | tr -d ' ')"
            if command -v shasum >/dev/null 2>&1; then
                printf 'gen1_elf_sha256=%s\n' "$(shasum -a 256 "$GEN1_ELF" | awk '{print $1}')"
            elif command -v sha256sum >/dev/null 2>&1; then
                printf 'gen1_elf_sha256=%s\n' "$(sha256sum "$GEN1_ELF" | awk '{print $1}')"
            fi
            if command -v file >/dev/null 2>&1; then
                printf '\n[file]\n'
                file "$GEN1_ELF" || true
            fi
            if command -v readelf >/dev/null 2>&1; then
                printf '\n[readelf -h]\n'
                readelf -h "$GEN1_ELF" || true
            fi
            if command -v readelf >/dev/null 2>&1; then
                printf '\n[readelf -l]\n'
                readelf -l "$GEN1_ELF" || true
            fi
        fi
        if [ -f "$log_path" ]; then
            printf '\n[log tail]\n'
            tail -n 80 "$log_path" || true
        fi
        if [ -f "$GEN1_ELF_LOG" ] && [ "$log_path" != "$GEN1_ELF_LOG" ]; then
            printf '\n[aggregate log tail]\n'
            tail -n 120 "$GEN1_ELF_LOG" || true
        fi
    } > "$GEN1_ELF_DIAG"
}

printf '=== Omgang 6b.3: stage-0 ELF sjølvkompilering ===\n\n'

printf '[1/4] Gen1: stage-0 NCB + ELF (host)...\n'
bash "$ROOT/tools/build_omgang6b_compiler_ncb.sh" "$GEN1_NCB"
bash "$ROOT/tools/ncb_to_elf.sh" "$GEN1_NCB" "$GEN1_ELF"
B1="$(wc -c < "$GEN1_ELF" | tr -d ' ')"
printf '  [OK] Gen1 ELF %s bytes\n\n' "$B1"

if [ "$OS" != "Linux" ] || { [ "$ARCH" != "x86_64" ] && [ "$ARCH" != "amd64" ]; }; then
    printf '[2–4/4] Hopp over ELF→ELF (%s/%s — krev Linux x86-64)\n' "$OS" "$ARCH"
    printf '\n=== Omgang 6b.3: DELVIS (host Gen1 bygd) ===\n'
    exit 0
fi

if [ "${NC_OM6B_RUN_STAGE0:-0}" != "1" ]; then
    printf '[2–4/4] Hopp over ELF→ELF runtime (set NC_OM6B_RUN_STAGE0=1 for djup smoke)\n'
    printf '\n=== Omgang 6b.3: DELVIS (Gen1 ELF bygd) ===\n'
    exit 0
fi

printf '[2/4] Gen2 NCB frå Gen1 ELF (bundle-args)...\n'
rm -f "$GEN2_NCB"
GEN2_CHUNK_DIR="$ROOT/build/6b/selfcompile"
GEN2_CHUNK_ENV="build/6b/selfcompile"
rm -f "$GEN2_CHUNK_DIR"/part_*.json
GEN2_ENTRY="selfhost.elf_compile_driver.start"
GEN2_BUNDLE_ARGS="__omgang6b__"

koyr_gen1_elf() {
    local mode="$1"
    local mode_log="$2"
    shift 2
    local -a cmd=("$@")
    printf '\n=== %s ===\n' "$mode" >> "$GEN1_ELF_LOG"
    set +e
    ("${cmd[@]}") 2>&1 | tee "$mode_log"
    local rc=$?
    set -e
    {
        printf 'exit_code=%s\n' "$rc"
        cat "$mode_log"
    } >> "$GEN1_ELF_LOG"
    printf 'mode=%s exit_code=%s log=%s\n' "$mode" "$rc" "$mode_log" >> "$GEN1_ELF_ATTEMPTS"
    return "$rc"
}

# Kjør Gen1-ELFen i eit reinvaska miljø for å unngå uventa testmiljøvariablar.
GEN1_ELF_PRESET_RUN=(
    env -i
    "PATH=$PATH"
    "NORSCODE_CMD=run"
    "NORSCODE_BUNDLE_ARGS=$GEN2_BUNDLE_ARGS"
    "NORSCODE_BUNDLE_OUTPUT=$GEN2_NCB"
    "NORSCODE_BUNDLE_OUTPUT_CHUNKS=1"
    "NORSCODE_BUNDLE_ENTRY=$GEN2_ENTRY"
    "$GEN1_ELF"
)

printf '  [INFO] Prøver ekte bundle-args via Gen1 ELF...\n'
if koyr_gen1_elf "bundle-args" "$GEN1_ELF_PRESET_LOG" "${GEN1_ELF_PRESET_RUN[@]}"; then
    if [ -f "$GEN2_NCB" ]; then
        printf '  [OK] Gen1 ELF skreiv Gen2 NCB via bundle-args\n'
    elif [ -f "$GEN2_CHUNK_DIR/part_000.json" ]; then
        cat "$GEN2_CHUNK_DIR"/part_*.json > "$GEN2_NCB"
        printf '  [OK] Gen1 ELF skreiv Gen2 NCB via bundle-args chunk-output\n'
    else
        printf '  [FEIL] Gen1 ELF returnerte suksess utan output\n' >&2
        exit 1
    fi
else
    _gen2_rc=$?
    if ls "$GEN2_CHUNK_DIR"/part_*.json >/dev/null 2>&1; then
        cat "$GEN2_CHUNK_DIR"/part_*.json > "$GEN2_NCB"
        printf '  [MERK] Bundle-args krasja med exit %d, men chunk-output vart skrive; brukar artifactet\n' "$_gen2_rc"
    else
        skriv_gen1_elf_diagnose "bundle_args" "$_gen2_rc" "env -i PATH=$PATH NORSCODE_CMD=run NORSCODE_BUNDLE_ARGS=\"$GEN2_BUNDLE_ARGS\" NORSCODE_BUNDLE_OUTPUT=$GEN2_NCB NORSCODE_BUNDLE_ENTRY=$GEN2_ENTRY $GEN1_ELF" "$GEN1_ELF_PRESET_LOG"
        printf '  [FEIL] Bundle-args feila med exit %d og Gen1 ELF skreiv ikkje Gen2 NCB\n' "$_gen2_rc" >&2
        printf '        Sjå diagnose: %s\n' "$GEN1_ELF_DIAG" >&2
        exit 1
    fi
fi

if [ ! -f "$GEN2_NCB" ]; then
    printf '  [FEIL] Gen2 NCB ikkje skrive\n' >&2
    exit 1
fi

N1="$(wc -c < "$GEN1_NCB" | tr -d ' ')"
N2="$(wc -c < "$GEN2_NCB" | tr -d ' ')"
printf '  [OK] Gen2 NCB %s bytes (Gen1 NCB %s bytes)\n' "$N2" "$N1"
if cmp -s "$GEN1_NCB" "$GEN2_NCB"; then
    printf '  [OK] NCB byte-paritet Gen1 == Gen2 (bundle-args)\n\n'
else
    printf '  [MERK] NCB differ i bundle-args-modus — sjekkar ELF-paritet likevel\n\n'
fi

printf '[3/4] Gen2 ELF frå Gen2 NCB (native codegen)...\n'
bash "$ROOT/tools/ncb_to_elf.sh" "$GEN2_NCB" "$GEN2_ELF"
B2="$(wc -c < "$GEN2_ELF" | tr -d ' ')"
printf '  [OK] Gen2 ELF %s bytes\n\n' "$B2"

printf '[4/4] Byte-paritet Gen1 ELF == Gen2 ELF (bundle-args)...\n'
if cmp -s "$GEN1_ELF" "$GEN2_ELF"; then
    printf '  [OK] %s bytes identiske\n\n' "$B1"
    printf 'bundle-args\n' > "$PASS_MARKER"
    printf '=== Omgang 6b.3: BESTÅTT (Gen1 ELF skreiv Gen2 NCB via bundle-args) ===\n'
    exit 0
fi

printf '  [FEIL] ELF differ (gen1=%s, gen2=%s bytes)\n' "$B1" "$B2" >&2
cmp -l "$GEN1_ELF" "$GEN2_ELF" 2>/dev/null | head -5 || true
exit 1
