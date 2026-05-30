"""Norscode CLI entrypoint (pip-installert).

AVVIKLA: primærbruk er bin/nc (Python-fri via nc-vm).
Denne modulen er berre for bakoverkompatibel pip-installasjon.
"""
from __future__ import annotations

import sys


def main_cli(argv: list[str] | None = None) -> int:
    from norcode.legacy_main import main as legacy_main
    legacy_main()
    return 0


modular_main_cli = main_cli
