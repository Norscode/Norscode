#!/usr/bin/env sh
# Norscode-first wrapper: legacy-CVM-gaten ligg i tools/no_legacy_cvm.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
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
file_has_legacy_ref() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *c_minimal_vm*|*ncbb*) return 0 ;;
    esac
  done < "$_file"
  return 1
}
if [ -z "${NORSCODE_NATIVE_BIN:-}" ] && [ ! -x "$ROOT/dist/norscode_native" ]; then
  case "$(uname -s):$(uname -m)" in
    Darwin:arm64) _stage0="$ROOT/bootstrap/stage0/norscode-macos-arm64" ;;
    Darwin:x86_64) _stage0="$ROOT/bootstrap/stage0/norscode-macos-x86_64" ;;
    Linux:x86_64) _stage0="$ROOT/bootstrap/stage0/norscode-linux-x86_64" ;;
    Linux:aarch64|Linux:arm64) _stage0="$ROOT/bootstrap/stage0/norscode-linux-arm64" ;;
    *) _stage0="" ;;
  esac
  if [ -n "$_stage0" ] && [ -x "$_stage0" ]; then
    export NORSCODE_NATIVE_BIN="$_stage0"
  fi
fi

_out="${TMPDIR:-/tmp}/no_legacy_cvm_$$.log"
_rc=0
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT" NORSCODE_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-}" "$ROOT/bin/nc" run "$ROOT/tools/no_legacy_cvm.no" >"$_out" 2>&1 || _rc=$?
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

if [ -e "$ROOT/tools/c_minimal_vm" ]; then
  printf 'Feil: tools/c_minimal_vm/ finst (legacy C-VM er fjerna). Sjå archive/c_minimal_vm/README.md\n'
  exit 1
fi
if [ -e "$ROOT/tools/build_norscode_native_from_source.sh" ]; then
  printf 'Feil: tools/build_norscode_native_from_source.sh finst (legacy). Bruk ./bin/nc fetch-stage0-seed\n'
  exit 1
fi
_hits=""
while IFS= read -r _file || [ -n "$_file" ]; do
  case "$_file" in
    "$ROOT/tools/no_legacy_cvm.sh"|"$ROOT/tools/no_legacy_cvm.no"|"$ROOT/tools/no_c_python_active_surface.sh"|"$ROOT/tools/no_c_python_active_surface.no")
      continue
      ;;
  esac
  if file_has_legacy_ref "$_file"; then
    _hits="${_hits}${_hits:+
}${_file}"
  fi
done <<EOF
$(find "$ROOT/tools" -type f \
  ! -name 'no_legacy_cvm.sh' \
  ! -name 'no_legacy_cvm.no' \
  ! -name 'no_c_python_active_surface.sh' \
  ! -name 'no_c_python_active_surface.no' \
  -print 2>/dev/null || true)
EOF
if [ -n "$_hits" ]; then
  printf 'Feil: historisk C-VM/NCBB-referanse i tools/:\n%s\n' "$_hits"
  exit 1
fi
printf 'OK: ingen historisk C-VM/NCBB under tools/\n'
