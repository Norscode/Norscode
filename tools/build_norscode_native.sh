#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Bygger dist/norscode_native.
# norscode_native er ein sjølvstendig binary kompilert frå Norscode-kjelda.
# Krev at dist/norscode_native allereie eksisterer (bootstrapping).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norscode_native"

# Dersom binæren allereie eksisterer, er det ingenting å gjere
if [ -x "$OUT" ]; then
    printf "✓ dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

printf "Feil: dist/norscode_native finst ikkje.\n" >&2
printf "Last ned ein pre-bygd release frå:\n" >&2
printf "  https://github.com/jansteinar/Norscode1/releases\n" >&2
printf "\nEller installer via:\n" >&2
printf "  curl -fsSL https://raw.githubusercontent.com/jansteinar/Norscode1/main/tools/install.sh | sh\n" >&2
exit 1
