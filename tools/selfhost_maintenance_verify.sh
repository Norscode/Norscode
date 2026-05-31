#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

require_match() {
  file="$1"
  pattern="$2"
  if ! grep -F -q -- "$pattern" "$file"; then
    printf 'Mangler forventet tekst i %s: %s\n' "$file" "$pattern" >&2
    exit 1
  fi
}

require_absent() {
  file="$1"
  pattern="$2"
  if grep -F -q -- "$pattern" "$file"; then
    printf 'Uønskt tekst i %s: %s\n' "$file" "$pattern" >&2
    exit 1
  fi
}

require_match "$ROOT_DIR/docs/START_HER.md" "docs/ARCHIVE_INDEX.md"
require_match "$ROOT_DIR/docs/HANDOFF.md" "ARCHIVE_INDEX.md"
require_match "$ROOT_DIR/docs/MAINTENANCE_POLICY.md" "docs/ARCHIVE_INDEX.md"
require_match "$ROOT_DIR/docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md" "ARCHIVE_INDEX"
require_match "$ROOT_DIR/docs/SELFHOST_STATUS.md" "docs/ARCHIVE_INDEX.md"

require_match "$ROOT_DIR/docs/SELFHOST_CI_GATES.md" "Normal CI for release/install skal ikke kreve C-verktøykjede."
require_match "$ROOT_DIR/docs/SELFHOST_RELEASE_CHECKLIST.md" "release/install-flaten krever ikke C-verktøykjede"
require_match "$ROOT_DIR/docs/SELFHOST_HANDLINGSPLAN.md" "release/install-flaten krever ikke C-verktøykjede"

require_absent "$ROOT_DIR/docs/SELFHOST_STATUS.md" "Neste konkrete patch"

printf 'Vedlikeholdsverifikasjon: OK\n'
