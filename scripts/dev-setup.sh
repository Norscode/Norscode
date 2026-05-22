#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if [ "${LEGACY_PYTHON:-0}" = "1" ]; then
  # Python-sporet: kun for utvikling på kompilatoren selv
  PYTHON_BIN="${PYTHON_BIN:-python3}"
  VENV_DIR="${VENV_DIR:-.venv}"

  "$PYTHON_BIN" -m venv "$VENV_DIR"
  # shellcheck disable=SC1090
  source "$VENV_DIR/bin/activate"

  python -m ensurepip --upgrade >/dev/null 2>&1 || true
  python -m pip install --upgrade pip setuptools wheel
  python -m pip install -e .

  echo
  echo "Python-utviklingsmiljø klart."
  echo "Aktiver miljøet med: source $VENV_DIR/bin/activate"
  echo "Merk: For vanlig bruk av Norscode trengs ikke Python. Bruk: ./bin/nc"
  exit 0
fi

# Standard installasjon: native bootstrap-kompilator (Python-fri)
bash tools/build-bootstrap-binary.sh

echo
echo "Norscode er klar (Python-fri)."
echo "Kjør CLI med: ./bin/nc run app.no"
echo ""
echo "Tips:"
echo "  ./bin/nc run app.no          # kjør en Norscode-fil"
echo "  ./bin/nc build app.no ut.elf # bygg til native binary"
echo "  ./bin/nc test                # kjør tester"
echo ""
echo "For å jobbe på kompilatoren selv: LEGACY_PYTHON=1 bash scripts/dev-setup.sh"
