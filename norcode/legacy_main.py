from __future__ import annotations

import sys

from norcode.bootstrap_support import _format_cli_exception
from norcode.command_dispatch import dispatch_command
from norcode.cli_parser import build_parser


def main() -> None:
    parser = build_parser()

    args = parser.parse_args()

    try:
        if getattr(args, "command_module", None) is not None:
            exit_code = dispatch_command(args)
            if exit_code != 0:
                sys.exit(exit_code)
            return

        parser.print_help()
        sys.exit(1)
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        sys.exit(1)


if __name__ == "__main__":
    main()
