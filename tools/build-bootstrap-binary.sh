#!/usr/bin/env bash
# tools/build-bootstrap-binary.sh
#
# Verifiserer at norscode_native eksisterer.
# norscode_native er den sjølvstendige Norscode-runtime (ingen C-kjelde nødvendig).
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
bash "${ROOT_DIR}/tools/build_norscode_native.sh"
