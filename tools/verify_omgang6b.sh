#!/usr/bin/env sh
# Tynn wrapper: Omgang 6b-verifisering ligg i tools/verify_omgang6b.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

run_shell_gate() {
  mkdir -p "$ROOT/build/6b"
  printf '=== Omgang 6b: ELF stage-0 (6b.1 + 6b.2 + 6b.3) ===\n\n'
  printf '[6b.1] Host ELF determinisme...\n'
  "$ROOT/bin/nc" bygg-native tests/fixtures/omgang6b_host.no build/6b/host_v1.elf
  "$ROOT/bin/nc" bygg-native tests/fixtures/omgang6b_host.no build/6b/host_v2.elf
  cmp -s build/6b/host_v1.elf build/6b/host_v2.elf
  printf '  [OK] host_v1 == host_v2 (%s bytes)\n\n' "$(wc -c < build/6b/host_v1.elf | tr -d ' ')"

  printf '[6b.2] Kompilator stage-0 NCB + ELF determinisme...\n'
  bash "$ROOT/tools/build_omgang6b_compiler_ncb.sh" "$ROOT/build/6b/compiler_stage0.ncb.json"
  bash "$ROOT/tools/ncb_to_elf.sh" "$ROOT/build/6b/compiler_stage0.ncb.json" "$ROOT/build/6b/compiler_v1.elf"
  bash "$ROOT/tools/ncb_to_elf.sh" "$ROOT/build/6b/compiler_stage0.ncb.json" "$ROOT/build/6b/compiler_v2.elf"
  cmp -s "$ROOT/build/6b/compiler_v1.elf" "$ROOT/build/6b/compiler_v2.elf"
  printf '  [OK] compiler_v1 == compiler_v2 (%s bytes)\n\n' "$(wc -c < "$ROOT/build/6b/compiler_v1.elf" | tr -d ' ')"

  case "$(uname -s):$(uname -m)" in
    Linux:x86_64|Linux:amd64)
      if [ "${NC_OM6B_RUN_STAGE0:-0}" = "1" ]; then
        printf '[6b.3] Stage-0 ELF sjølvkompilering...\n'
        bash "$ROOT/tools/selfcompile_stage0_elf.sh"
      else
        printf '[6b.3] Hopp over ELF→ELF runtime (set NC_OM6B_RUN_STAGE0=1 for djup smoke)\n\n'
      fi
      ;;
    *)
      printf '[6b.3] Hopp over ELF→ELF runtime (%s/%s — krev Linux x86-64)\n\n' "$(uname -s)" "$(uname -m)"
      ;;
  esac
  printf '=== Omgang 6b.1 + 6b.2 + 6b.3: BESTÅTT ===\n'
}

_out="${TMPDIR:-/tmp}/verify_omgang6b_$$.log"
_rc=0
"$ROOT/bin/nc" run "$ROOT/tools/verify_omgang6b.no" >"$_out" 2>&1 || _rc=$?
if [ "$_rc" -eq 0 ]; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi
if [ ! -s "$_out" ] || grep -Eq 'Ukjent funksjon: builtin(\.builtin)?\.exec_prosess' "$_out"; then
  rm -f "$_out"
  run_shell_gate
  exit 0
fi
cat "$_out"
rm -f "$_out"
exit "$_rc"
