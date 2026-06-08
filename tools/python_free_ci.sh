#!/bin/sh
# tools/python_free_ci.sh — Python- og C-fri CI-gate for normal flyt
#
# 1. Verifiserer at normal overflate er utan Python/C
# 2. Sikrar dist/norscode_native (nedlasting eller eksisterande)
# 3. Køyrer test_*.no via bin/nc
#
# Krev: curl eller wget (for nedlasting), sh — ikkje Python, ikkje clang i denne gaten

set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Norscode Python-fri CI ===\n\n'

# 0. Normal overflate må vere fri for Python/C (og andre legacy-referansar)
bash tools/no_c_python_active_surface.sh

# 1. Stage-0 native
printf '1. dist/norscode_native...\n'
bash tools/build_norscode_native.sh 2>&1 | tail -2
printf '\n'

# 2. Tester
printf '2. Køyrer testar...\n'
sh tools/nc_test.sh

printf '\n=== Python-fri CI: BESTÅTT ===\n'
