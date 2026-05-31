#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

bash "$ROOT_DIR/tools/selfhost_phase0_verify.sh"
bash "$ROOT_DIR/tools/selfhost_maintenance_verify.sh"

printf 'Driftsvakt: OK\n'
