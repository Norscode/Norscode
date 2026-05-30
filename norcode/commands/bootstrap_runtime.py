"""AVVIKLA: erstatta av nc-vm. Bruk bin/nc i staden."""
from __future__ import annotations
from norcode.commands.base import CommandModule

def _noop_register(parser) -> None: pass

def _noop_run(args) -> int:
    print("AVVIKLA: bruk bin/nc")
    return 2

BOOTSTRAP_COMPILER_VERIFY_COMMAND = CommandModule(name="bootstrap-compiler-verify", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
REGISTRY_HOST_COMMAND = CommandModule(name="registry-host", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
SELFHOST_CHAIN_RUN_COMMAND = CommandModule(name="selfhost-chain-run", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
SELFHOST_COMPILE_ALL_COMMAND = CommandModule(name="selfhost-compile-all", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
