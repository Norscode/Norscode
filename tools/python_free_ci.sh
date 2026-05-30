#!/bin/sh
# tools/python_free_ci.sh — Python-fri CI: bygg + test utan Python
#
# Køyrer: bygg norscode_native → testar 48 test_*.no
# Krev kun: clang (eller cc)

set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Norscode Python-fri CI ===\n\n'

# 1. Bygg norscode_native
printf '1. Byggjer dist/norscode_native...\n'
bash tools/build_norscode_native.sh 2>&1 | tail -2
printf '\n'

# 2. Køyr testar
printf '2. Køyrer testar...\n'
sh tools/nc_test.sh

printf '\n=== Python-fri CI: BESTÅTT ===\n'
