#!/usr/bin/env bash
# tools/no_legacy_cvm.sh — fail if legacy C-VM paths reappear under tools/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [ -d "$ROOT/tools/c_minimal_vm" ]; then
    printf 'Feil: tools/c_minimal_vm/ finst (legacy C-VM er fjerna). Sjå archive/c_minimal_vm/README.md\n' >&2
    exit 1
fi

if [ -f "$ROOT/tools/build_norscode_native_from_source.sh" ]; then
    printf 'Feil: tools/build_norscode_native_from_source.sh finst (legacy). Bruk tools/build_norscode_native.sh\n' >&2
    exit 1
fi

_grep_tools() {
    local pattern="$1"
    if command -v rg >/dev/null 2>&1; then
        rg -l "$pattern" "$ROOT/tools" --glob '!no_legacy_cvm.sh' 2>/dev/null || true
    else
        grep -rl "$pattern" "$ROOT/tools" 2>/dev/null | grep -v 'no_legacy_cvm\.sh$' || true
    fi
}

hits="$(_grep_tools 'c_minimal_vm')"
if [ -n "$hits" ]; then
    printf 'Feil: referanse til c_minimal_vm i tools/:\n%s\n' "$hits" >&2
    exit 1
fi

hits="$(_grep_tools 'ncbb')"
if [ -n "$hits" ]; then
    printf 'Feil: referanse til ncbb (legacy binærformat) i tools/:\n%s\n' "$hits" >&2
    exit 1
fi

printf 'OK: ingen legacy C-VM/NCBB under tools/\n'
