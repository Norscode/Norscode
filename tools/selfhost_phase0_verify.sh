#!/usr/bin/env sh
# Norscode-first wrapper: fase-0-gaten ligg i tools/selfhost_phase0_verify.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
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
phase0_skip_file() {
  _name="${1##*/}"
  case "$_name" in
    selfhost_phase0_verify.sh|selfhost_phase0_verify.no|enforce_native_first.sh|enforce_native_first.no|nc_test.sh|nc_test.no) return 0 ;;
  esac
  return 1
}
phase0_rel() {
  case "$1" in
    "$ROOT_DIR"/*) printf '%s' "${1#"$ROOT_DIR"/}" ;;
    *) printf '%s' "$1" ;;
  esac
}
phase0_scan_file() {
  _file="$1"
  phase0_skip_file "$_file" && return 0
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *main.py*) printf '%s: main.py\n' "$(phase0_rel "$_file")" ;;
      *compile.py*) printf '%s: compile.py\n' "$(phase0_rel "$_file")" ;;
      *"docker buildx build"*) printf '%s: docker buildx build\n' "$(phase0_rel "$_file")" ;;
      *Dockerfile.linux-build*) printf '%s: Dockerfile.linux-build\n' "$(phase0_rel "$_file")" ;;
      *setup.py*) printf '%s: setup.py\n' "$(phase0_rel "$_file")" ;;
      *test_web_dependency.no*) printf '%s: test_web_dependency.no\n' "$(phase0_rel "$_file")" ;;
      *"Bygg dist/norscode for normal bruk, eller bruk ./bin/bootstrap."*) printf '%s: Bygg dist/norscode for normal bruk, eller bruk ./bin/bootstrap.\n' "$(phase0_rel "$_file")" ;;
      *dist/norcode-bootstrap-compile*) printf '%s: dist/norcode-bootstrap-compile\n' "$(phase0_rel "$_file")" ;;
    esac
  done < "$_file"
}
phase0_scan_path() {
  _path="$1"
  if [ -f "$_path" ]; then
    phase0_scan_file "$_path"
    return 0
  fi
  if [ -d "$_path" ]; then
    _files="$(find "$_path" -path "$ROOT_DIR/.git" -prune -o -type f -print 2>/dev/null || true)"
    while IFS= read -r _file || [ -n "$_file" ]; do
      [ -n "$_file" ] && phase0_scan_file "$_file"
    done <<EOF
$_files
EOF
  fi
}
if [ -z "${NORSCODE_NATIVE_BIN:-}" ] && [ ! -x "$ROOT_DIR/dist/norscode_native" ]; then
  case "$(uname -s):$(uname -m)" in
    Darwin:arm64) _stage0="$ROOT_DIR/bootstrap/stage0/norscode-macos-arm64" ;;
    Darwin:x86_64) _stage0="$ROOT_DIR/bootstrap/stage0/norscode-macos-x86_64" ;;
    Linux:x86_64) _stage0="$ROOT_DIR/bootstrap/stage0/norscode-linux-x86_64" ;;
    Linux:aarch64|Linux:arm64) _stage0="$ROOT_DIR/bootstrap/stage0/norscode-linux-arm64" ;;
    *) _stage0="" ;;
  esac
  if [ -n "$_stage0" ] && [ -x "$_stage0" ]; then
    export NORSCODE_NATIVE_BIN="$_stage0"
  fi
fi
_out="${TMPDIR:-/tmp}/selfhost_phase0_verify_$$.log"
_rc=0
env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT_DIR" NORSCODE_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-}" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/selfhost_phase0_verify.no" >"$_out" 2>&1 || _rc=$?
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

sh -n \
  "$ROOT_DIR/bin/nc" \
  "$ROOT_DIR/bin/bootstrap" \
  "$ROOT_DIR/bin/nl" \
  "$ROOT_DIR/bin/nor" \
  "$ROOT_DIR/tools/build-bootstrap-binary.sh" \
  "$ROOT_DIR/tools/install.sh" \
  "$ROOT_DIR/tools/docker-build-linux.sh" \
  "$ROOT_DIR/tools/selfhost_phase0_verify.sh" \
  "$ROOT_DIR/tools/verify_norscode_surface_ownership.sh"

"$ROOT_DIR/bin/nc" surface-ownership

_hits="$(
  phase0_scan_path "$ROOT_DIR/README.md"
  phase0_scan_path "$ROOT_DIR/.github/workflows"
  phase0_scan_path "$ROOT_DIR/bin"
  phase0_scan_path "$ROOT_DIR/tools"
  phase0_scan_path "$ROOT_DIR/docs/05-development/SELFHOST_FALLBACK_CONTRACT.md"
  phase0_scan_path "$ROOT_DIR/docs/05-development/SELFHOST_CI_GATES.md"
  phase0_scan_path "$ROOT_DIR/docs/05-development/SELFHOST_RELEASE_CHECKLIST.md"
  phase0_scan_path "$ROOT_DIR/docs/README.md"
  phase0_scan_path "$ROOT_DIR/docs/INDEX.md"
  phase0_scan_path "$ROOT_DIR/docs/SELFHOST_HANDLINGSPLAN.md"
  phase0_scan_path "$ROOT_DIR/docs/STATUS.md"
)"
if [ -n "$_hits" ]; then
  printf 'Fase 0-verifisering feila: historiske blokkere finst framleis i aktiv flate.\n%s\n' "$_hits"
  exit 1
fi
printf 'Fase 0-verifisering: OK\n'
printf 'Aktiv flate er fri for kjende historiske fase-0-blokkere.\n'
