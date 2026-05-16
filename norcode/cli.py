"""Primary Norscode CLI entrypoint.

The default entrypoint still forwards to the legacy Python bootstrap so the
full existing CLI remains compatible.  A modular CLI entrypoint is also exposed
for the commands that have already been moved to `norcode.commands`.
"""

from __future__ import annotations

import sys

from norcode.bootstrap.python_entry import bootstrap_main
from norcode.command_dispatch import CommandDispatchError, dispatch_command
from norcode.cli_parser import build_parser



def main_cli() -> None:
    bootstrap_main()



def modular_main_cli(argv: list[str] | None = None) -> int:
    """Run the new modular CLI path for migrated commands.

    This is intentionally separate from `main_cli()` until the full CLI surface
    has moved out of `main.py`.
    """
    parser = build_parser()
    args = parser.parse_args(argv)
    if getattr(args, "cmd", None) is None:
        parser.print_help()
        return 2
    try:
        return dispatch_command(args)
    except CommandDispatchError as exc:
        print(str(exc), file=sys.stderr)
        return 1
