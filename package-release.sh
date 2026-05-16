#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COMMON_SCRIPT="${SCRIPT_DIR}/../../scripts/package-release-common.sh"
BOOTSTRAP_BUILD_SCRIPT="${SCRIPT_DIR}/tools/build-bootstrap-binary.sh"

if [ -x "${BOOTSTRAP_BUILD_SCRIPT}" ]; then
  "${BOOTSTRAP_BUILD_SCRIPT}"
fi

if [ ! -f "${COMMON_SCRIPT}" ]; then
  printf 'Fant ikke felles release-script: %s\n' "${COMMON_SCRIPT}" >&2
  exit 1
fi

PROJECT_ROOT="${SCRIPT_DIR}"
PROJECT_NAME="norscode-language"
ENTRYPOINT="README.md"

REQUIRED_PATHS=(
  "main.py"
  "norcode"
  "norsklang"
  "compiler"
  "selfhost"
  "selfhost_parity"
  "std"
  "packages"
  "docs"
  "scripts"
  "README.md"
  "LICENSE"
  "CHANGELOG.md"
  "pyproject.toml"
  "setup.py"
  "bin"
)

OPTIONAL_PATHS=(
  "examples"
  "tests"
  "website"
  "dist"
  "app.c"
  "app.no"
  "README.md"
  "norcode.toml"
  "app.lock"
  "lock"
  "registry"
)

EXCLUDE_PATHS=(
  ".git"
  ".github"
  ".vscode"
  ".venv"
  ".build-linux-venv"
  ".build-macos-venv"
  "__pycache__"
  ".DS_Store"
  "release-artifacts"
)

# Wrapperen beskriver hvilke filer prosjektet trenger; felles logikk bor i scripts/.
# shellcheck source=../../scripts/package-release-common.sh
source "${COMMON_SCRIPT}"

if [ "$#" -gt 0 ]; then
  package_release "$@"
else
  package_release
fi
