#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"

_out="${TMPDIR:-/tmp}/verify_selvstendighet_$$.log"
_rc=0
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/verify_selvstendighet.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi

if [ -s "$_out" ] && ! grep -Eq 'Ukjent funksjon: builtin(\.builtin)?\.exec_prosess' "$_out"; then
  cat "$_out"
  rm -f "$_out"
  exit "$_rc"
fi
rm -f "$_out"

cd "$ROOT"
printf '=== Norscode sjølvstendighet (normalflate, L1-L6) ===\n\n'

printf '0. Python-gate i tools/...\n'
bash tools/python_dependency_audit.sh
printf '\n'

printf '0b. Ingen aktiv C/Python-flate...\n'
bash tools/no_c_python_active_surface.sh
printf '\n'

printf '0c. Ingen legacy C-VM under tools/...\n'
bash tools/no_legacy_cvm.sh
printf '\n'

printf '1. Aktiv tools-flate har ingen C/Python-kjelder...\n'
if find "$ROOT/tools" -type f \( -name '*.c' -o -name '*.h' -o -name '*.py' \) | grep . >/dev/null 2>&1; then
  printf '  [FEIL] C/Python funne under tools/\n'
  exit 1
fi
printf '  [OK] tools/ er C/Python-fri\n\n'

if [ ! -x "$ROOT/dist/norscode_native" ]; then
  printf '  [FEIL] dist/norscode_native manglar. Normalflate skal ikkje bygge stage-0 her.\n'
  printf '         Materialiser frå seed/release med: bash tools/build_norscode_native.sh\n'
  exit 1
fi
printf '2. Stage-0: dist/norscode_native finst alt (ingen rebuild i normalflate)\n\n'

printf '3. Selfhost bootstrap-gate (steg A+B)...\n'
"$ROOT/bin/nc" selfhost-bootstrap-gate
printf '\n'

printf '4. Bootstrap-self (steg C)...\n'
"$ROOT/bin/nc" bootstrap-self
printf '\n'

printf '5. L5 sjølvkompilering (Gen1 == Gen2)...\n'
mkdir -p "$ROOT/build/l5"
bash tools/selfcompile_l5.sh
printf '\n'

printf '6. L5b-smoke er djupare paritet og ikkje del av kort normal verifisering.\n'
printf '   Køyr ved behov: bash tools/selfcompile_l5b_mini.sh\n\n'
printf '7. Testsuite: full testflate kan køyrast separat.\n'
printf '   Køyr ved behov: ./bin/nc test\n\n'
printf '=== Sjølvstendighet L1-L6 (normalflate): BESTÅTT ===\n'
