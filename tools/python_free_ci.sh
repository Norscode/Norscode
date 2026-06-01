#!/bin/sh
# tools/python_free_ci.sh — Python- og C-fri CI-gate for normal flyt
#
# 1. Verifiserer at tools/ ikkje har nye .py-filer
# 2. Sikrar dist/norscode_native (nedlasting eller eksisterande)
# 3. Køyrer test_*.no via bin/nc
#
# Krev: curl eller wget (for nedlasting), sh — ikkje Python, ikkje clang i denne gaten

set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Norscode Python-fri CI ===\n\n'

# 0. Ingen Python i tools/ (normalflate)
_py_count="$(find tools -maxdepth 1 -name '*.py' 2>/dev/null | wc -l | tr -d ' ')"
if [ "$_py_count" != "0" ]; then
    printf 'Feil: fant %s Python-fil(er) i tools/ — normal flyt er kun Norscode.\n' "$_py_count" >&2
    find tools -maxdepth 1 -name '*.py' >&2
    exit 1
fi
printf '0. tools/: ingen .py  [OK]\n\n'

# 1. Stage-0 native
printf '1. dist/norscode_native...\n'
bash tools/build_norscode_native.sh 2>&1 | tail -2
printf '\n'

# 2. Tester
printf '2. Køyrer testar...\n'
sh tools/nc_test.sh

printf '\n=== Python-fri CI: BESTÅTT ===\n'
