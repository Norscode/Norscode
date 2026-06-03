#!/usr/bin/env bash
# tools/patch_ncb_entry.sh — sett NCB entry-felt (Omgang 6b)
set -euo pipefail

NCB="${1:?bruk: patch_ncb_entry.sh fil.ncb.json entry.funksjon}"
ENTRY="${2:?}"

if ! grep -Fq "\"$ENTRY\":{" "$NCB"; then
    printf "Feil: entry %s finst ikkje i bundle\n" "$ENTRY" >&2
    exit 1
fi

if ! grep -Eq '"entry"[[:space:]]*:' "$NCB"; then
    printf 'Feil: manglar entry-felt i %s\n' "$NCB" >&2
    exit 1
fi

NC_ENTRY="$ENTRY" perl -0pi -e 's/"entry"[[:space:]]*:[[:space:]]*"[^"]*"/"entry":"$ENV{NC_ENTRY}"/' "$NCB"
printf '  entry → %s\n' "$ENTRY"
