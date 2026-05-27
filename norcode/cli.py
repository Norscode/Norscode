"""Primary Norscode CLI entrypoint.

The default entrypoint now uses the modular command registry directly.
"""

from __future__ import annotations

import sys

from norcode.command_dispatch import dispatch_command
from norcode.cli_parser import build_parser


def main_cli(argv: list[str] | None = None) -> int:
    """Run the default CLI entrypoint.

    This now uses the modular command registry first, and only falls back to
    the legacy Python bootstrap for commands that have not been migrated yet.
    """
    return modular_main_cli(sys.argv[1:] if argv is None else argv)


def modular_main_cli(argv: list[str] | None = None) -> int:
    """Run the modular CLI path for commands in the registry."""
    parser = build_parser()
    args = parser.parse_args(argv)
    if getattr(args, "cmd", None) is None:
        parser.print_help()
        return 2

    exit_code = dispatch_command(args)
    return 0 if exit_code == 0 else exit_code
