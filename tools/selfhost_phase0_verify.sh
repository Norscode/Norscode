#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ACTIVE_ROOTS=(
  "$ROOT_DIR/README.md"
  "$ROOT_DIR/.github/workflows"
  "$ROOT_DIR/bin"
  "$ROOT_DIR/tools"
  "$ROOT_DIR/docs/05-development/SELFHOST_FALLBACK_CONTRACT.md"
  "$ROOT_DIR/docs/05-development/SELFHOST_CI_GATES.md"
  "$ROOT_DIR/docs/05-development/SELFHOST_RELEASE_CHECKLIST.md"
  "$ROOT_DIR/docs/README.md"
  "$ROOT_DIR/docs/INDEX.md"
  "$ROOT_DIR/docs/SELFHOST_HANDLINGSPLAN.md"
  "$ROOT_DIR/docs/STATUS.md"
)

check_sh_syntax() {
  sh -n \
    "$ROOT_DIR/bin/nc" \
    "$ROOT_DIR/bin/bootstrap" \
    "$ROOT_DIR/bin/nl" \
    "$ROOT_DIR/bin/nor" \
    "$ROOT_DIR/tools/build-bootstrap-binary.sh" \
    "$ROOT_DIR/tools/install.sh" \
    "$ROOT_DIR/tools/docker-build-linux.sh" \
    "$ROOT_DIR/tools/selfhost_phase0_verify.sh"
}

check_active_surface() {
  local active_files=()
  local path
  for path in "${ACTIVE_ROOTS[@]}"; do
    if [ -f "$path" ] || [ -d "$path" ]; then
      active_files+=("$path")
    fi
  done

  grep -rn \
    "main\.py\|compile\.py\|docker buildx build\|Dockerfile\.linux-build\|setup\.py\|test_web_dependency\.no\|Bygg dist/norscode for normal bruk, eller bruk \./bin/bootstrap\." \
    "${active_files[@]}" \
    --exclude="selfhost_phase0_verify.sh" \
    --exclude="enforce_native_first.sh" \
    --exclude="nc_test.sh" \
    --exclude-dir=".git" \
    2>/dev/null
}

main() {
  check_sh_syntax
  if check_active_surface >/dev/null; then
    printf 'Fase 0-verifisering feila: historiske blokkere finst framleis i aktiv flate.\n' >&2
    check_active_surface >&2 || true
    exit 1
  fi

  printf 'Fase 0-verifisering: OK\n'
  printf 'Aktiv flate er fri for kjente historiske fase-0-blokkere.\n'
}

main "$@"
