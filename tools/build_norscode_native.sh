#!/usr/bin/env bash
# tools/build_norscode_native.sh
#
# Verifiserer at dist/norscode_native eksisterer.
# norscode_native er den sjølvstendige Norscode-runtime (ingen C-kjelde nødvendig).
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT_DIR}/dist/norscode_native"

if [ -x "$OUT" ]; then
    printf "✓ dist/norscode_native er allereie bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
    exit 0
fi

printf "Feil: dist/norscode_native finst ikkje.\n" >&2
printf "Last ned frå: https://github.com/jansteinar/Norscode1/releases\n" >&2
exit 1
