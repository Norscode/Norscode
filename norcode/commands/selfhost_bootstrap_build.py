"""AVVIKLA: erstatta av nc-vm. Bruk bin/nc i staden."""
from __future__ import annotations
from norcode.commands.base import CommandModule

def _noop_register(parser) -> None: pass

def _noop_run(args) -> int:
    print("AVVIKLA: bruk bin/nc")
    return 2

SELFHOST_BOOTSTRAP_BUILD_COMMAND = CommandModule(name="selfhost-build", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
