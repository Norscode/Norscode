#!/usr/bin/env sh
set -eu
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
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

sh -n \
  "$ROOT_DIR/bin/nc" \
  "$ROOT_DIR/bin/bootstrap" \
  "$ROOT_DIR/bin/nl" \
  "$ROOT_DIR/bin/nor" \
  "$ROOT_DIR/tools/build-bootstrap-binary.sh" \
  "$ROOT_DIR/tools/install.sh" \
  "$ROOT_DIR/tools/docker-build-linux.sh" \
  "$ROOT_DIR/tools/selfhost_phase0_verify.sh"

_pattern='main\.py\|compile\.py\|docker buildx build\|Dockerfile\.linux-build\|setup\.py\|test_web_dependency\.no\|Bygg dist/norscode for normal bruk, eller bruk \./bin/bootstrap\.'
_hits="$(grep -rn "$_pattern" \
  "$ROOT_DIR/README.md" \
  "$ROOT_DIR/.github/workflows" \
  "$ROOT_DIR/bin" \
  "$ROOT_DIR/tools" \
  "$ROOT_DIR/docs/05-development/SELFHOST_FALLBACK_CONTRACT.md" \
  "$ROOT_DIR/docs/05-development/SELFHOST_CI_GATES.md" \
  "$ROOT_DIR/docs/05-development/SELFHOST_RELEASE_CHECKLIST.md" \
  "$ROOT_DIR/docs/README.md" \
  "$ROOT_DIR/docs/INDEX.md" \
  "$ROOT_DIR/docs/SELFHOST_HANDLINGSPLAN.md" \
  "$ROOT_DIR/docs/STATUS.md" \
  --exclude='selfhost_phase0_verify.sh' \
  --exclude='selfhost_phase0_verify.no' \
  --exclude='enforce_native_first.sh' \
  --exclude='enforce_native_first.no' \
  --exclude='nc_test.sh' \
  --exclude='nc_test.no' \
  --exclude-dir='.git' 2>/dev/null || true)"
if [ -n "$_hits" ]; then
  printf 'Fase 0-verifisering feila: historiske blokkere finst framleis i aktiv flate.\n%s\n' "$_hits"
  exit 1
fi
printf 'Fase 0-verifisering: OK\n'
printf 'Aktiv flate er fri for kjente historiske fase-0-blokkere.\n'
