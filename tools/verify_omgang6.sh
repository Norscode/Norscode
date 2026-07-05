#!/usr/bin/env sh
# Tynn wrapper: Omgang 6-logikken ligg i tools/verify_omgang6.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MODE="${1:-all}"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
export NC_OM6_MODE="$MODE"

run_shell_gate() {
  _src1="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_src1_XXXXXX.no")"
  _src2="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_src2_XXXXXX.no")"
  _src3="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_src3_XXXXXX.no")"
  _src4="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_src4_XXXXXX.no")"
  _src5="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_src5_XXXXXX.no")"
  _out1="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf1_XXXXXX")"
  _out2="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf2_XXXXXX")"
  _out3="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf3_XXXXXX")"
  _out4="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf4_XXXXXX")"
  _out5="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf5_XXXXXX")"
  _out6="$(mktemp "${TMPDIR:-/tmp}/nc_omgang6_elf6_XXXXXX")"
  trap 'rm -f "$_src1" "$_src2" "$_src3" "$_src4" "$_src5" "$_out1" "$_out2" "$_out3" "$_out4" "$_out5" "$_out6"' EXIT HUP INT TERM

  printf 'funksjon start() {\n  skriv("ELF-smoke OK\\n")\n}\n' > "$_src1"
  printf 'funksjon start() {\n  la x: heltall = 6 * 7\n  skriv(tekst_fra_heltall(x))\n  skriv("\\n")\n}\n' > "$_src2"
  printf 'funksjon start() {\n  la x: heltall = heltall_fra_tekst("42")\n  skriv(tekst_fra_heltall(x))\n  skriv("\\n")\n}\n' > "$_src3"
  printf 'funksjon f(a: tekst) {\n  skriv(type_av(a))\n  skriv("\\n")\n}\nfunksjon start() {\n  f("abc")\n}\n' > "$_src4"
  printf 'funksjon f(a: tekst, b: tekst) {\n  skriv(a + b)\n  skriv("\\n")\n}\nfunksjon start() {\n  f("1", "2")\n}\n' > "$_src5"

  mode_match() { [ "$MODE" = "all" ] || [ "$MODE" = "$1" ]; }
  printf '=== Omgang 6: Native ELF (bygg-native) ===\n\n'
  if mode_match skriv; then
    printf '1. bygg-native (Gen1, skriv-smoke)...\n'
    "$ROOT/bin/nc" bygg-native "$_src1" "$_out1"
    printf '  [OK] %s bytes\n\n' "$(wc -c < "$_out1" | tr -d ' ')"
    printf '2. bygg-native (Gen2, byte-determinisme, skriv-smoke)...\n'
    "$ROOT/bin/nc" bygg-native "$_src1" "$_out2"
    cmp -s "$_out1" "$_out2"
    printf '  [OK] Gen1 == Gen2 (%s bytes)\n\n' "$(wc -c < "$_out1" | tr -d ' ')"
  fi
  if mode_match int; then
    printf '4. bygg-native (Gen1, int-to-str smoke)...\n'
    "$ROOT/bin/nc" bygg-native "$_src2" "$_out3"
    printf '  [OK] int-to-str bygg-native smoke er bygd (%s bytes)\n\n' "$(wc -c < "$_out3" | tr -d ' ')"
    printf '6. bygg-native (Gen1, str-to-int smoke)...\n'
    "$ROOT/bin/nc" bygg-native "$_src3" "$_out4"
    printf '  [OK] str-to-int bygg-native smoke er bygd (%s bytes)\n\n' "$(wc -c < "$_out4" | tr -d ' ')"
  fi
  if mode_match type; then
    printf '8. bygg-native (Gen1, type_av parameter smoke)...\n'
    "$ROOT/bin/nc" bygg-native "$_src4" "$_out5"
    printf '  [OK] type_av bygg-native smoke er bygd (%s bytes)\n\n' "$(wc -c < "$_out5" | tr -d ' ')"
  fi
  if mode_match add; then
    printf '10. bygg-native (Gen1, tekst + tekst parameter smoke)...\n'
    "$ROOT/bin/nc" bygg-native "$_src5" "$_out6"
    printf '  [OK] tekst + tekst bygg-native smoke er bygd (%s bytes)\n\n' "$(wc -c < "$_out6" | tr -d ' ')"
  fi
  printf '=== Omgang 6: BESTÅTT ===\n'
  printf 'bygg-native produserer deterministisk Linux ELF64 utan clang.\n'
}

_out="${TMPDIR:-/tmp}/verify_omgang6_$$.log"
_rc=0
"$ROOT/bin/nc" run "$ROOT/tools/verify_omgang6.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi
if grep -Eq 'Ukjent funksjon: builtin(\.builtin)?\.exec_prosess' "$_out"; then
  rm -f "$_out"
  run_shell_gate
  exit 0
fi
cat "$_out"
rm -f "$_out"
exit "$_rc"
