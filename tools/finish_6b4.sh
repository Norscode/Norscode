#!/usr/bin/env bash
# tools/finish_6b4.sh — avslutt Omgang 6b.4: fjern committed bootstrap/c/*.c
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

MAC="$ROOT/bootstrap/stage0/norscode-macos-arm64"
LIN="$ROOT/bootstrap/stage0/norscode-linux-x86_64"

missing=0
for f in "$MAC" "$LIN"; do
    if [ ! -f "$f" ]; then
        printf 'Manglar: %s\n' "$f" >&2
        missing=1
    fi
done

if [ "$missing" -eq 1 ]; then
    printf '\nHent Linux-seed:\n' >&2
    printf '  GitHub → Actions → Export stage0 Linux seed → last ned artefakt\n' >&2
    printf '  cp norscode-linux-x86_64 bootstrap/stage0/\n' >&2
    printf '  eller: bash tools/migrate_bootstrap_c_to_stage0.sh (på Linux)\n' >&2
    exit 1
fi

printf '=== Omgang 6b.4: fjern bootstrap/c/*.c ===\n\n'

if git ls-files --error-unmatch bootstrap/c/norscode_generated.c >/dev/null 2>&1; then
    git rm -f bootstrap/c/norscode_generated.c bootstrap/c/nc_dispatch.c
    printf '  git rm bootstrap/c/*.c\n'
else
    printf '  bootstrap/c/*.c er ikkje i git (OK)\n'
fi

printf '\nVerifiser seed-only (macOS):\n'
CC=__clang_not_allowed__ REGEN=0 bash "$ROOT/tools/build_norscode_native.sh"
printf '\n=== Klar — commit og push. CI skal ikkje trenge clang for ensure (berre regen-lane). ===\n'
