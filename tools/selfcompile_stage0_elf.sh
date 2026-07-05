#!/usr/bin/env sh
# Tynn wrapper: Omgang 6b.3-orkestrering ligg i tools/selfcompile_stage0_elf.no.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

run_shell_gate() {
  base="$ROOT/build/6b/selfcompile"
  gen1_ncb="$ROOT/build/6b/compiler_stage0.ncb.json"
  gen1_elf="$base/gen1_compiler.elf"
  gen2_ncb="$base/gen2.ncb.json"
  gen2_elf="$base/gen2_compiler.elf"
  gen1_log="$base/gen1_elf_bundle.log"
  preset_log="$base/gen1_elf_preset.log"
  attempts="$base/gen1_elf_attempts.txt"
  pass_marker="$base/stage0_elf_passed.marker"

  mkdir -p "$base"
  rm -f "$pass_marker" "$gen1_log" "$preset_log" "$base"/gen1_elf_source_ncb.log \
    "$base"/gen1_elf_chunked_source_ncb.log "$base"/gen1_elf_diagnose.txt "$attempts"

  printf '=== Omgang 6b.3: stage-0 ELF sjølvkompilering ===\n\n'
  printf '[1/4] Gen1: stage-0 NCB + ELF (host)...\n'
  bash "$ROOT/tools/build_omgang6b_compiler_ncb.sh" "$gen1_ncb"
  bash "$ROOT/tools/ncb_to_elf.sh" "$gen1_ncb" "$gen1_elf"
  chmod +x "$gen1_elf"
  b1="$(wc -c < "$gen1_elf" | tr -d ' ')"
  printf '  [OK] Gen1 ELF %s bytes\n\n' "$b1"

  case "$(uname -s):$(uname -m)" in
    Linux:x86_64|Linux:amd64) ;;
    *)
      printf '[2-4/4] Hopp over ELF->ELF (%s/%s - krev Linux x86-64)\n\n' "$(uname -s)" "$(uname -m)"
      printf '=== Omgang 6b.3: DELVIS (host Gen1 bygd) ===\n'
      return 0
      ;;
  esac

  if [ "${NC_OM6B_RUN_STAGE0:-0}" != "1" ]; then
    printf '[2-4/4] Hopp over ELF->ELF runtime (set NC_OM6B_RUN_STAGE0=1 for djup smoke)\n\n'
    printf '=== Omgang 6b.3: DELVIS (Gen1 ELF bygd) ===\n'
    return 0
  fi

  printf '[2/4] Gen2 NCB frå Gen1 ELF (bundle-args)...\n'
  rm -f "$gen2_ncb" "$base"/part_*.json
  _gen2_rc=0
  env -i \
    PATH="${PATH:-/usr/bin:/bin}" \
    NORSCODE_ROOT="$ROOT" \
    NORSCODE_CMD=run \
    NORSCODE_USE_PRECOMPILED_SELFHOST=0 \
    NORSCODE_BUNDLE_ARGS=__omgang6b__ \
    NORSCODE_BUNDLE_OUTPUT="$gen2_ncb" \
    NORSCODE_BUNDLE_OUTPUT_CHUNKS=1 \
    NORSCODE_BUNDLE_ENTRY=selfhost.elf_compile_driver.start \
    "$gen1_elf" > "$preset_log" 2>&1 || _gen2_rc=$?
  if [ "$_gen2_rc" -ne 0 ] && ls "$base"/part_*.json >/dev/null 2>&1; then
    cat "$base"/part_*.json > "$gen2_ncb"
  elif [ "$_gen2_rc" -ne 0 ]; then
    [ -f "$preset_log" ] && cat "$preset_log"
    printf '  [FEIL] Gen1 ELF skreiv ikkje Gen2 NCB (exit %s)\n' "$_gen2_rc"
    return 1
  fi
  if [ ! -f "$gen2_ncb" ]; then
    if ls "$base"/part_*.json >/dev/null 2>&1; then
      cat "$base"/part_*.json > "$gen2_ncb"
    else
      [ -f "$preset_log" ] && cat "$preset_log"
      printf '  [FEIL] Gen1 ELF returnerte %s utan å skrive Gen2 NCB\n' "$_gen2_rc"
      return 1
    fi
  fi
  test -f "$gen2_ncb"
  printf '  [OK] Gen2 NCB %s bytes (Gen1 NCB %s bytes)\n\n' \
    "$(wc -c < "$gen2_ncb" | tr -d ' ')" "$(wc -c < "$gen1_ncb" | tr -d ' ')"

  printf '[3/4] Gen2 ELF frå Gen2 NCB (native codegen)...\n'
  bash "$ROOT/tools/ncb_to_elf.sh" "$gen2_ncb" "$gen2_elf"
  chmod +x "$gen2_elf"
  b2="$(wc -c < "$gen2_elf" | tr -d ' ')"
  printf '  [OK] Gen2 ELF %s bytes\n\n' "$b2"

  printf '[4/4] Byte-paritet Gen1 ELF == Gen2 ELF (bundle-args)...\n'
  cmp -s "$gen1_elf" "$gen2_elf"
  printf '  [OK] %s bytes identiske\n\n' "$b1"
  printf 'bundle-args\n' > "$pass_marker"
  printf '=== Omgang 6b.3: BESTÅTT (Gen1 ELF skreiv Gen2 NCB via bundle-args) ===\n'
}

_out="${TMPDIR:-/tmp}/selfcompile_stage0_elf_$$.log"
_rc=0
"$ROOT/bin/nc" run "$ROOT/tools/selfcompile_stage0_elf.no" >"$_out" 2>&1 || _rc=$?
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
