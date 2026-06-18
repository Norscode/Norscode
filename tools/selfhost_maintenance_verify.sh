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

require_match "$ROOT_DIR/docs/INDEX.md" "docs/_archive/ARCHIVE_INDEX.md"
require_match "$ROOT_DIR/docs/README.md" "docs/SELFHOST_HANDLINGSPLAN.md"
require_match "$ROOT_DIR/docs/05-development/SELFHOST_MIGRATION_AND_DEPRECATIONS.md" "ARCHIVE_INDEX"
require_match "$ROOT_DIR/docs/STATUS.md" "docs/_archive/"

require_match "$ROOT_DIR/docs/05-development/SELFHOST_CI_GATES.md" "Normal CI for release/install skal ikkje krevje C-verktøykjede"
require_match "$ROOT_DIR/docs/05-development/SELFHOST_RELEASE_CHECKLIST.md" "release/install-flaten krever ikke C-verktøykjede"
require_match "$ROOT_DIR/docs/SELFHOST_HANDLINGSPLAN.md" "C-regen skal berre finnast i eksplisitt vedlikehaldslane"

require_absent "$ROOT_DIR/docs/STATUS.md" "Neste konkrete patch"

printf 'Vedlikeholdsverifikasjon: OK\n'
