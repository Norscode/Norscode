#!/usr/bin/env sh
# Historisk Norscode-first wrapper: sjølvstendighetsloggen ligg i tools/verify_selvstendighet.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
STRICT=0
if [ "${1:-}" = "--strict" ]; then
  STRICT=1
  shift
fi
if [ "$#" -ne 0 ]; then
  printf 'bruk: ./bin/nc selvstendighet [--strict]\n' >&2
  exit 2
fi

print_file() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    printf '%s\n' "$_line"
  done < "$_file"
}
has_exec_gap() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

_out="${TMPDIR:-/tmp}/verify_selvstendighet_$$.log"
_rc=0
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_SELVSTENDIGHET_STRICT="$STRICT" NORSCODE_ROOT="$ROOT" "$ROOT/bin/nc" run "$ROOT/tools/verify_selvstendighet.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  print_file "$_out"
  rm -f "$_out"
  exit 0
fi

if [ -s "$_out" ] && ! has_exec_gap "$_out"; then
  print_file "$_out"
  rm -f "$_out"
  exit "$_rc"
fi
rm -f "$_out"

cd "$ROOT"
printf '=== Norscode sjølvstendighet (normalflate, L1-L6) ===\n\n'

printf '0. Python-gate i tools/...\n'
"$ROOT/bin/nc" python-audit
printf '\n'

printf '0b. Ingen aktiv C/Python-flate...\n'
"$ROOT/bin/nc" active-surface
printf '\n'

printf '0c. Ingen historisk C-VM under tools/...\n'
"$ROOT/bin/nc" no-legacy-cvm
printf '\n'

printf '1. Aktiv tools-flate har ingen C/Python-kjelder...\n'
_tools_c_python="$(find "$ROOT/tools" -type f \( -name '*.c' -o -name '*.h' -o -name '*.py' \) -print -quit 2>/dev/null || true)"
if [ -n "$_tools_c_python" ]; then
  printf '  [FEIL] C/Python funne under tools/\n'
  printf '         %s\n' "$_tools_c_python"
  exit 1
fi
printf '  [OK] tools/ er C/Python-fri\n\n'

if [ ! -x "$ROOT/dist/norscode_native" ]; then
  printf '  [FEIL] dist/norscode_native manglar. Normalflate skal ikkje bygge stage-0 her.\n'
  printf '         Materialiser frå seed/release med: ./bin/nc fetch-stage0-seed\n'
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
"$ROOT/bin/nc" selfcompile-l5
printf '\n'

if [ "$STRICT" = "1" ]; then
  printf '6. L5b-mini VM-paritet...\n'
  "$ROOT/bin/nc" selfcompile-l5b-mini
  printf '\n7. Full L5b VM-paritet...\n'
  "$ROOT/bin/nc" selfcompile-l5b
  printf '\n8. Stage-0 seed-integritet...\n'
  "$ROOT/bin/nc" verify-seed
  printf '\n9. Bygg isolert single-binary-kandidat (ingen promotering)...\n'
  NORSCODE_EMBED_BASE="$ROOT/dist/norscode_native" \
    NORSCODE_EMBED_OUT="$ROOT/build/l5b/norscode_native_embedded" \
    NORSCODE_EMBED_STREAM=1 \
    NORSCODE_EMBED_SPECS="compiler=$ROOT/build/l5/compiler_v1.ncb.json
vm=$ROOT/bootstrap/precompiled/vm.ncb.json" \
    "$ROOT/bin/nc" run "$ROOT/tools/embed_release_ncb.no"
  printf '\n9b. Single-binary utan repo/NCB/PATH...\n'
  NORSCODE_NATIVE_BIN="$ROOT/build/l5b/norscode_native_embedded" "$ROOT/bin/nc" run "$ROOT/tools/single_binary_gate.no"
  printf '\n10. Full normal testflate...\n'
  "$ROOT/bin/nc" test
  printf '\n11. Langsam testflate...\n'
  NC_SLOW_TESTS=1 TEST_TIMEOUT=600 SLOW_TEST_TIMEOUT=600 "$ROOT/bin/nc" test
  printf '\n=== Streng sjølvstendighet: BESTÅTT ===\n'
  exit 0
fi
printf '6. L5b og full testflate er ikkje del av kort normal verifisering.\n'
printf '   Køyr full bevisflate med: ./bin/nc selvstendighet --strict\n\n'
printf '=== Sjølvstendighet L1-L6 (normalflate): BESTÅTT ===\n'
