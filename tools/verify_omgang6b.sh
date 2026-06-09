#!/usr/bin/env bash
# tools/verify_omgang6b.sh — Omgang 6b.1 + 6b.2 + 6b.3: NCB → ELF stage-0, compile, transitional sjølvkompilering
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OS="$(uname -s)"
ARCH="$(uname -m)"

ncb_entry() {
    local path="$1"
    local line
    line="$(grep -o '"entry":"[^"]*"' "$path" | head -1 || true)"
    line="${line#\"entry\":\"}"
    line="${line%\"}"
    printf '%s' "$line"
}

ncb_function_count() {
    local path="$1"
    grep -o '":{"name"' "$path" | wc -l | tr -d ' '
}

printf '=== Omgang 6b: ELF stage-0 (6b.1 + 6b.2 + 6b.3) ===\n\n'

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

mkdir -p build/6b

# ─── 6b.1a: standalone host-ELF ───────────────────────────────────────────────
printf '1a. Host-ELF frå omgang6b_host.no...\n'
"$ROOT/bin/nc" bygg-native tests/fixtures/omgang6b_host.no build/6b/host_v1.elf
"$ROOT/bin/nc" bygg-native tests/fixtures/omgang6b_host.no build/6b/host_v2.elf
chmod +x build/6b/host_v1.elf build/6b/host_v2.elf
if ! cmp -s build/6b/host_v1.elf build/6b/host_v2.elf; then
    printf '  [FEIL] host ELF ikkje deterministisk\n' >&2
    exit 1
fi
printf '  [OK] host ELF deterministisk (%d bytes)\n\n' "$(wc -c < build/6b/host_v1.elf | tr -d ' ')"

# ─── 6b.1b + 6b.2: kompilator stage-0 NCB → ELF ─────────────────────────────
printf '1b. Stage-0 NCB (driver entry) → ELF...\n'
bash "$ROOT/tools/build_omgang6b_compiler_ncb.sh" build/6b/compiler_stage0.ncb.json
ENTRY="$(ncb_entry build/6b/compiler_stage0.ncb.json)"
if [ "$ENTRY" != "selfhost.elf_compile_driver.start" ]; then
    printf '  [FEIL] entry er %s, forventa selfhost.elf_compile_driver.start\n' "$ENTRY" >&2
    exit 1
fi
bash "$ROOT/tools/ncb_to_elf.sh" build/6b/compiler_stage0.ncb.json build/6b/compiler_v1.elf
bash "$ROOT/tools/ncb_to_elf.sh" build/6b/compiler_stage0.ncb.json build/6b/compiler_v2.elf
chmod +x build/6b/compiler_v1.elf build/6b/compiler_v2.elf
if ! cmp -s build/6b/compiler_v1.elf build/6b/compiler_v2.elf; then
    printf '  [FEIL] kompilator-ELF ikkje deterministisk\n' >&2
    exit 1
fi
printf '  [OK] stage-0 ELF deterministisk (%d bytes)\n\n' "$(wc -c < build/6b/compiler_v1.elf | tr -d ' ')"

# ─── Linux: køyr ELF ──────────────────────────────────────────────────────────
if [ "$OS" = "Linux" ] && { [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "amd64" ]; }; then
    printf '2. Køyr host-ELF på Linux...\n'
    OUT="$(build/6b/host_v1.elf)"
    printf '%s' "$OUT"
    echo "$OUT" | grep -q "6b-elf-host OK" || { printf '  [FEIL] host-ELF output\n' >&2; exit 1; }
    printf '  [OK] host-ELF\n\n'

    if [ "${NC_OM6B_RUN_STAGE0:-0}" = "1" ]; then
        printf '3. Køyr stage-0 ELF (røyk-modus utan env)...\n'
        COUT="$(build/6b/compiler_v1.elf 2>&1)" || {
            printf '  [FEIL] stage-0 ELF exit %d\n' "$?" >&2
            printf '%s\n' "$COUT" >&2
            exit 1
        }
        printf '%s' "$COUT"
        echo "$COUT" | grep -q "ELF compile driver" || { printf '  [FEIL] manglar driver-røyk\n' >&2; exit 1; }
        echo "$COUT" | grep -q "inline compile" || { printf '  [FEIL] manglar inline compile\n' >&2; exit 1; }
        printf '  [OK] stage-0 ELF røyk-modus\n\n'

        printf '4. Kompiler eksternt .no via NORSCODE_FILE...\n'
        rm -f build/6b/target.ncb.json
        export NORSCODE_FILE="$ROOT/tests/fixtures/omgang6b_target.no"
        export NORSCODE_OUTPUT="$ROOT/build/6b/target.ncb.json"
        export NORSCODE_MODULE="__main__"
        COUT2="$(build/6b/compiler_v1.elf 2>&1)" || {
            printf '  [FEIL] ekstern compile exit %d\n' "$?" >&2
            printf '%s\n' "$COUT2" >&2
            exit 1
        }
        printf '%s' "$COUT2"
        echo "$COUT2" | grep -q "ELF compile:" || { printf '  [FEIL] manglar compile-linje\n' >&2; exit 1; }
        if [ ! -f build/6b/target.ncb.json ]; then
            printf '  [FEIL] target.ncb.json ikkje skrive\n' >&2
            exit 1
        fi
        if ! grep -q '"functions":{' build/6b/target.ncb.json; then
            printf '  [FEIL] target NCB manglar functions\n' >&2
            exit 1
        fi
        if ! grep -Eq '"[^"]*start":\{' build/6b/target.ncb.json; then
            printf '  [FEIL] target NCB manglar start\n' >&2
            exit 1
        fi
        printf '  [OK] target NCB: %s funksjonar\n' "$(ncb_function_count build/6b/target.ncb.json)"
        printf '  [OK] ekstern .no → NCB via stage-0 ELF\n\n'
        unset NORSCODE_FILE NORSCODE_OUTPUT NORSCODE_MODULE
    else
        printf '3–4. Hopp over stage-0 ELF runtime (set NC_OM6B_RUN_STAGE0=1 for djup smoke)\n\n'
    fi
else
    printf '2–4. Hopp over ELF-køyring (%s/%s — krev Linux x86-64)\n\n' "$OS" "$ARCH"
fi

# ─── 6b.3: Gen1 ELF → Gen2 ELF byte-paritet ───────────────────────────────────
printf '5. Stage-0/6b.3 sjølvkompilering (transitional source-ncb / ekte ELF-bundle når tilgjengeleg)...\n'
bash "$ROOT/tools/selfcompile_stage0_elf.sh"
printf '\n'

printf '=== Omgang 6b.1 + 6b.2 + 6b.3: BESTÅTT ===\n'
printf 'NCB → ELF deterministisk; dyp stage-0 ELF runtime er opt-in med NC_OM6B_RUN_STAGE0=1.\n'
printf 'Merk: 6b.3 brukar for tida transitional source-NCB-løype i staden for hard-gata ekte intern ELF-bundle.\n'
printf 'Neste: commit bootstrap/stage0/norscode-linux-x86_64 og fjern bootstrap/maint/c/*.c frå git.\n'
