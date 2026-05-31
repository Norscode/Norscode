#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# Standard installasjon: native bootstrap-kompilator
bash tools/build-bootstrap-binary.sh

echo
echo "Norscode er klar."
echo "Kjør CLI med: ./bin/nc run app.no"
echo ""
echo "Tips:"
echo "  ./bin/nc run app.no          # kjør en Norscode-fil"
echo "  ./bin/nc build app.no ut.elf # bygg til native binary"
echo "  ./bin/nc test                # kjør tester"
echo ""
echo "For å jobbe på kompilatoren selv: bruk native bootstrap-verktøyene i tools/"
