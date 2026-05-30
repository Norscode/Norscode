"""AVVIKLA: erstatta av nc-vm (Python-fri). Bruk bin/nc i staden."""
from __future__ import annotations
from norcode.commands.base import CommandModule

def _noop_register(parser) -> None:
    pass

def _noop_run(args) -> int:
    import subprocess
    nc = __import__("subprocess")
    print(f"AVVIKLA: '" + name + "' er erstatta. Bruk: bin/nc run/compile/test")
    return 2

BUILD_BYTECODE_COMMAND = CommandModule(
    name="bytecode-build",
    help="[avvikla — bruk bin/nc]",
    register_arguments=_noop_register,
    run=_noop_run,
)
