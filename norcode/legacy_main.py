"""Norscode pip-entrypoint. Delegerer til bin/nc (Python-fri via nc-vm)."""
from __future__ import annotations
import os
import subprocess
import sys

_KOMMANDOAR = {
    "run", "compile", "build", "check", "format", "lint",
    "doctor", "repl", "test", "selfcheck", "identity",
    "selfhost-bootstrap-gate", "ci", "commands", "help",
}

_FJERNA = {
    "serve", "smoke", "bench", "migrate-names", "diagnose",
    "stress", "fuzz", "add", "update", "lock", "packages", "scaffold-api",
}


def main() -> None:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    nc = os.path.join(root, "bin", "nc")

    if len(sys.argv) < 2 or sys.argv[1] in ("--help", "-h"):
        subprocess.run([nc, "help"])
        return

    cmd = sys.argv[1]

    if cmd in _FJERNA:
        print(f"nc {cmd}: avvikla og fjerna.", file=sys.stderr)
        sys.exit(2)

    result = subprocess.run([nc] + sys.argv[1:])
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
