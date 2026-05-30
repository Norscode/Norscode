"""legacy_main.py — AVVIKLA. Python-fallback er fjerna.

Alle Norscode-kommandoar er Python-fri via bin/nc (nc-vm).
Denne fila finst berre for pip-installert bakoverkompatibilitet.
"""
from __future__ import annotations

import subprocess
import sys
import os


_PYTHON_FRI = {
    "run", "compile", "build", "check", "format", "lint",
    "doctor", "repl", "test", "selfcheck", "identity",
    "selfhost-bootstrap-gate", "ci",
}

_AVVIKLA_FJERNA = {
    "serve", "smoke", "bench", "migrate-names", "diagnose",
    "stress", "fuzz", "add", "update", "lock", "packages", "scaffold-api",
}


def main() -> None:
    if len(sys.argv) < 2 or sys.argv[1] in ("--help", "-h", "help"):
        print("norcode – Norscode CLI\n")
        print("Primærbruk: bin/nc <kommando>  (Python-fri via nc-vm)\n")
        print("Python-fri kommandoar:", ", ".join(sorted(_PYTHON_FRI)))
        sys.exit(0)

    cmd = sys.argv[1]

    if cmd == "test":
        root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        result = subprocess.run(
            ["sh", os.path.join(root, "tools", "nc_test.sh")] + sys.argv[2:]
        )
        sys.exit(result.returncode)

    if cmd in _PYTHON_FRI:
        root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        nc = os.path.join(root, "bin", "nc")
        result = subprocess.run([nc] + sys.argv[1:])
        sys.exit(result.returncode)

    if cmd in _AVVIKLA_FJERNA:
        print(
            f"nc {cmd}: avvikla og fjerna. Python-fallback er ikkje lenger tilgjengeleg.",
            file=sys.stderr,
        )
        sys.exit(2)

    print(
        f"norcode: ukjend kommando '{cmd}'. Bruk bin/nc <kommando>.",
        file=sys.stderr,
    )
    sys.exit(2)


if __name__ == "__main__":
    main()
