#!/usr/bin/env bash
# tools/update_stage0_manifest.sh — lag/reproduser SHA256-manifest for bootstrap/stage0
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OUT="bootstrap/stage0/SHA256SUMS"
TMP="${OUT}.tmp"

if ! command -v sha256sum >/dev/null 2>&1 && ! command -v shasum >/dev/null 2>&1; then
    printf 'Feil: verken sha256sum eller shasum er tilgjengeleg\n' >&2
    exit 1
fi

printf 'Lagar manifest for stage0-seedar i bootstrap/stage0...\n'
: > "$TMP"
for bin in bootstrap/stage0/norscode-*; do
    [ -f "$bin" ] || continue
    case "$(basename "$bin")" in
        *.sha* ) continue ;;
        SHA256SUMS* ) continue ;;
    esac
    if command -v sha256sum >/dev/null 2>&1; then
        sha=$(sha256sum "$bin" | awk '{print $1}')
    else
        sha=$(shasum -a 256 "$bin" | awk '{print $1}')
    fi
    printf '%s  %s\n' "$sha" "$bin" >> "$TMP"
done

if [ -s "$TMP" ]; then
    sort "$TMP" -o "$TMP"
    mv "$TMP" "$OUT"
    printf '✓ Manifest oppdatert: %s\n' "$OUT"
    sed -n '1,20p' "$OUT"
else
    rm -f "$TMP"
    : > "$OUT"
    printf '✓ Ingen seed-filer funne; oppretta tomt manifest: %s\n' "$OUT"
fi
