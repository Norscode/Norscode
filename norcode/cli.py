"""Primary Norscode CLI entrypoint.

The default entrypoint now prefers the modular CLI path for migrated commands
and falls back to the legacy Python bootstrap only for commands that still
live in `main.py`.
"""

from __future__ import annotations

import sys

from norcode.bootstrap.python_entry import bootstrap_main
from norcode.command_dispatch import CommandDispatchError, dispatch_command
from norcode.cli_parser import build_parser



def main_cli() -> None:
    parser = build_parser()
    args = parser.parse_args(sys.argv[1:])
    if getattr(args, "cmd", None) is None:
        parser.print_help()
        return

    command = getattr(args, "command_module", None)
    if command is not None and command.run is not None:
        exit_code = dispatch_command(args)
        if exit_code != 0:
            raise SystemExit(exit_code)
        return

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
