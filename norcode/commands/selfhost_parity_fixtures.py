"""AVVIKLA: Paritets-fiksturas erstatta av nc-vm."""
from __future__ import annotations
from norcode.commands.base import CommandModule
def _noop_register(parser): pass
def _noop_run(args): print("AVVIKLA"); return 2
UPDATE_SELFHOST_PARITY_FIXTURES_COMMAND = CommandModule(name="update-selfhost-parity-fixtures", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
SYNC_SELFHOST_PARITY_M2_COMMAND = CommandModule(name="sync-selfhost-parity-m2", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
