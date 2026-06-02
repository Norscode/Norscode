#!/usr/bin/env bash
# tools/verify_omgang6b.sh — Omgang 6b.1: NCB → ELF stage-0 grunnlag
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OS="$(uname -s)"
ARCH="$(uname -m)"

printf '=== Omgang 6b.1: ELF stage-0 grunnlag ===\n\n'

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

mkdir -p build/6b

# ─── 6b.1a: standalone host-ELF ───────────────────────────────────────────────
printf '1a. Host-ELF frå omgang6b_host.no...\n'
"$ROOT/bin/nc" bygg-native tests/fixtures/omgang6b_host.no build/6b/host_v1.elf
"$ROOT/bin/nc" bygg-native tests/fixtures/omgang6b_host.no build/6b/host_v2.elf
if ! cmp -s build/6b/host_v1.elf build/6b/host_v2.elf; then
    printf '  [FEIL] host ELF ikkje deterministisk\n' >&2
    exit 1
fi
printf '  [OK] host ELF deterministisk (%d bytes)\n\n' "$(wc -c < build/6b/host_v1.elf | tr -d ' ')"

# ─── 6b.1b: kompilator-kjede NCB → ELF ────────────────────────────────────────
printf '1b. Kompilator-kjede NCB → ELF...\n'
bash "$ROOT/tools/build_omgang6b_compiler_ncb.sh" build/6b/compiler_chain.ncb.json
bash "$ROOT/tools/ncb_to_elf.sh" build/6b/compiler_chain.ncb.json build/6b/compiler_v1.elf
bash "$ROOT/tools/ncb_to_elf.sh" build/6b/compiler_chain.ncb.json build/6b/compiler_v2.elf
if ! cmp -s build/6b/compiler_v1.elf build/6b/compiler_v2.elf; then
    printf '  [FEIL] kompilator-ELF ikkje deterministisk\n' >&2
    exit 1
fi
printf '  [OK] kompilator-ELF deterministisk (%d bytes)\n\n' "$(wc -c < build/6b/compiler_v1.elf | tr -d ' ')"

# ─── Linux: køyr ELF ──────────────────────────────────────────────────────────
if [ "$OS" = "Linux" ] && { [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "amd64" ]; }; then
    printf '2. Køyr host-ELF på Linux...\n'
    OUT="$(build/6b/host_v1.elf)"
    printf '%s' "$OUT"
    echo "$OUT" | grep -q "6b-elf-host OK" || { printf '  [FEIL] host-ELF output\n' >&2; exit 1; }
    echo "$OUT" | grep -q "7" || { printf '  [FEIL] host-ELF 3+4\n' >&2; exit 1; }
    printf '  [OK] host-ELF\n\n'

    printf '3. Køyr kompilator-kjede ELF (røyk-test)...\n'
    COUT="$(build/6b/compiler_v1.elf 2>&1)" || {
        printf '  [FEIL] kompilator-ELF exit %d\n' "$?" >&2
        printf '%s\n' "$COUT" >&2
        exit 1
    }
    printf '%s' "$COUT"
    echo "$COUT" | grep -q "Selfhost-kompilator" || { printf '  [FEIL] manglar Selfhost-kompilator\n' >&2; exit 1; }
    echo "$COUT" | grep -q "Røyk-test" || { printf '  [FEIL] manglar Røyk-test\n' >&2; exit 1; }
    printf '  [OK] kompilator-ELF køyrer røyk-test\n\n'
else
    printf '2–3. Hopp over ELF-køyring (%s/%s — krev Linux x86-64)\n\n' "$OS" "$ARCH"
fi

printf '=== Omgang 6b.1: BESTÅTT ===\n'
printf 'NCB → ELF for host og kompilator-kjede er deterministisk.\n'
printf 'Neste (6b.2): ELF klarer compile av eksternt .no-program.\n'
