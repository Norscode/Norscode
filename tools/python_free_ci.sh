#!/bin/sh
# tools/python_free_ci.sh — Python-fri CI: bygg + test utan Python
#
# Køyrer: bygg nc-vm → oppdater stdlib → testar 48 test_*.no
# Krev kun: clang (eller cc)

set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

printf '=== Norscode Python-fri CI ===\n\n'

# 1. Bygg nc-vm
printf '1. Byggjer dist/nc-vm...\n'
sh tools/bootstrap.sh 2>&1 | tail -4
printf '\n'

# 2. Køyr testar
printf '2. Køyrer testar...\n'
sh tools/nc_test.sh

printf '\n=== Python-fri CI: BESTÅTT ===\n'
