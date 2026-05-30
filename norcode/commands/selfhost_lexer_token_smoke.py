"""AVVIKLA: Erstatta av nc-vm (Python-fri). Bruk bin/nc i staden."""
from __future__ import annotations
from norcode.commands.base import CommandModule

def _noop_register(parser) -> None:
    pass

def _noop_run(args) -> int:
    import subprocess, sys
    print(f"AVVIKLA: 'selfhost-lexer-token-smoke' er erstatta av Python-fri nc-vm.")
    print("Bruk: bin/nc selfhost-bootstrap-gate  ELLER  bin/nc test")
    return 2

SELFHOST_LEXER_TOKEN_SMOKE_COMMAND = CommandModule(
    name="selfhost-lexer-token-smoke",
    help="[avvikla — bruk bin/nc]",
    register_arguments=_noop_register,
    run=_noop_run,
)
