#!/usr/bin/env sh
# Norscode-first wrapper: feature-gate-logikken ligg i tools/norscode_feature_gate.no.
# Shell-delen under er avgrensa reserveveg som berre kallar nc-flater.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

NORSCODE_FEATURE_FILES=""
for f in "$@"; do
    NORSCODE_FEATURE_FILES="${NORSCODE_FEATURE_FILES}${f}
"
done

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NORSCODE_FEATURE_FILES

OUT="${TMPDIR:-/tmp}/norscode_feature_gate_$$.log"
RC=0
cleanup() {
  rm -f "$OUT"
}
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
trap cleanup EXIT

"$ROOT/bin/nc" run "$ROOT/tools/norscode_feature_gate.no" >"$OUT" 2>&1 || RC=$?
if [ "$RC" -eq 0 ]; then
  print_file "$OUT"
  exit 0
fi

if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  exit "$RC"
fi

printf '=== Norscode feature-check (utan C/Python) ===\n\n'

printf '1. Policyflate...\n'
"$ROOT/bin/nc" active-surface >/dev/null
printf '  [OK] ingen aktiv C/Python-flate\n\n'

printf '2. Seed/runtime...\n'
"$ROOT/bin/nc" verify-seed >/dev/null
printf '  [OK] stage-0 seed og runtime\n\n'

if [ -n "$NORSCODE_FEATURE_FILES" ]; then
  printf '3. Sjekkar oppgitte Norscode-filer...\n'
  printf '%s' "$NORSCODE_FEATURE_FILES" | while IFS= read -r f; do
    [ -n "$f" ] || continue
    case "$f" in
      *.no)
        "$ROOT/bin/nc" check "$f" >/dev/null
        printf '  [OK] %s\n' "$f"
        ;;
      *)
        printf '  [HOPP] %s (ikkje .no)\n' "$f"
        ;;
    esac
  done
else
  printf '3. Ingen filer oppgitt; køyrer normal testflate...\n'
  "$ROOT/bin/nc" test >/dev/null
fi

printf '\n=== Feature-check: BESTÅTT ===\n'
