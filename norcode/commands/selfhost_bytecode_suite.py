"""AVVIKLA: Erstatta av nc-vm (Python-fri). Bruk bin/nc i staden."""
from __future__ import annotations
from norcode.commands.base import CommandModule

def _noop_register(parser) -> None:
    pass

def _noop_run(args) -> int:
    import subprocess, sys
    print(f"AVVIKLA: 'selfhost-bytecode-suite' er erstatta av Python-fri nc-vm.")
    print("Bruk: bin/nc selfhost-bootstrap-gate  ELLER  bin/nc test")
    return 2

SELFHOST_BYTECODE_SUITE_COMMAND = CommandModule(
    name="selfhost-bytecode-suite",
    help="[avvikla — bruk bin/nc]",
    register_arguments=_noop_register,
    run=_noop_run,
)
