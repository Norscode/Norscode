#!/usr/bin/env sh
# tools/norscode_feature_gate.sh — sjekk ny Norscode-funksjonalitet utan C/Python
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    printf 'Feil: trenger dist/norscode_native. Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

printf '=== Norscode feature-check (utan C/Python) ===\n\n'

printf '1. Policyflate...\n'
bash tools/no_c_python_active_surface.sh >/dev/null
printf '  [OK] ingen aktiv C/Python-flate\n\n'

printf '2. Seed/runtime...\n'
bash tools/verify_seed_only.sh >/dev/null
printf '  [OK] stage-0 seed og runtime\n\n'

if [ "$#" -gt 0 ]; then
    printf '3. Sjekkar oppgitte Norscode-filer...\n'
    for f in "$@"; do
        case "$f" in
            *.no)
                ./bin/nc check "$f" >/dev/null
                printf '  [OK] %s\n' "$f"
                ;;
            *)
                printf '  [HOPP] %s (ikkje .no)\n' "$f"
                ;;
        esac
    done
else
    printf '3. Ingen filer oppgitt; køyrer normal testflate...\n'
    ./bin/nc test
fi

printf '\n=== Feature-check: BESTÅTT ===\n'
