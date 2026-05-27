#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ACTIVE_ROOTS=(
  "$ROOT_DIR/README.md"
  "$ROOT_DIR/.github/workflows"
  "$ROOT_DIR/bin"
  "$ROOT_DIR/tools"
  "$ROOT_DIR/docs/START_HER.md"
  "$ROOT_DIR/docs/HANDOFF.md"
  "$ROOT_DIR/docs/CLI_CONTRACT.md"
  "$ROOT_DIR/docs/SELFHOST_FALLBACK_CONTRACT.md"
  "$ROOT_DIR/docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md"
  "$ROOT_DIR/docs/SELFHOST_CI_GATES.md"
  "$ROOT_DIR/docs/SELFHOST_RELEASE_CHECKLIST.md"
  "$ROOT_DIR/docs/MAINTENANCE_POLICY.md"
  "$ROOT_DIR/docs/PYTHON_UTFASING.md"
  "$ROOT_DIR/docs/python-phaseout.md"
  "$ROOT_DIR/docs/SELFSTENDIG_NORSCODE_ROADMAP.md"
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
  rg -n \
    "python3 main.py|docker buildx build|Dockerfile\.linux-build|setup\.py|test_web_dependency\.no" \
    "${ACTIVE_ROOTS[@]}" \
    -g '!selfhost_phase0_verify.sh' \
    -g '!**/.git/**'
}

check_legacy_flag_present() {
  rg -n -- "--legacy-python-fallback" "$ROOT_DIR/bin/nc" "$ROOT_DIR/bin/bootstrap" "$ROOT_DIR/docs"
}

main() {
  check_sh_syntax
  check_legacy_flag_present >/dev/null
  if check_active_surface >/dev/null; then
    printf 'Fase 0-verifisering feila: gamle blokkere finst framleis i aktiv flate.\n' >&2
    exit 1
  fi

  printf 'Fase 0-verifisering: OK\n'
  printf 'Aktiv flate er fri for kjente fase-0-blokkere.\n'
}

main "$@"
