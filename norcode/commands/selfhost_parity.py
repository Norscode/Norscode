"""AVVIKLA: Paritets-sjekkar erstatta av nc-vm + bin/nc test."""
from __future__ import annotations
from norcode.commands.base import CommandModule

def _noop_register(parser) -> None:
    pass

def _noop_run(args) -> int:
    print("AVVIKLA: Bruk bin/nc test (Python-fri)")
    return 2

SELFHOST_PARITY_COMMAND = CommandModule(name="selfhost-parity", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
SELFHOST_PARITY_PROGRESS_COMMAND = CommandModule(name="selfhost-parity-progress", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
SELFHOST_PARITY_GATE_COMMAND = CommandModule(name="selfhost-parity-gate", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
SELFHOST_PARITY_CONSISTENCY_COMMAND = CommandModule(name="selfhost-parity-consistency", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
