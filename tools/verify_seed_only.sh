#!/usr/bin/env bash
# tools/verify_seed_only.sh — verifiser seed-first utan clang/regen
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Seed-only verifikasjon (utan clang/regen) ===\n'
printf '1. Bygg frå seed med ugyldig CC (skal framleis passere)\n'
CC=__clang_not_allowed__ REGEN=0 bash "$ROOT/tools/build_norscode_native.sh"
printf '\n'

printf '2. Køyr native testløp\n'
sh "$ROOT/tools/nc_test.sh"
printf '\n'

printf '=== Seed-only verifikasjon: BESTÅTT ===\n'
