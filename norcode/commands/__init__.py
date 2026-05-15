"""Command-layer package for the Norscode CLI.

Commands are currently still executed by the legacy Python bootstrap in
`main.py`.  This package is the migration target for moving command
registration and handlers out of `main.py` step by step.
"""

from .registry import COMMANDS, CommandSpec, command_names, command_overview

__all__ = ["COMMANDS", "CommandSpec", "command_names", "command_overview"]
