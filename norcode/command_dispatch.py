"""Command dispatch bridge for the modular CLI.

Command modules may expose a `run(args)` handler, while commands without a
handler can still be marked as bootstrap-only.  The dispatch layer keeps that
distinction explicit so the modular CLI and the legacy wrapper can share the
same registry without duplicating execution logic.
"""

from __future__ import annotations

from typing import Any


class CommandDispatchError(RuntimeError):
    pass


def dispatch_command(args: Any) -> int:
    """Dispatch a parsed command module.

    Returns a process-style exit code.  A missing handler is not silently
    ignored; it tells the caller that this command is still owned by the legacy
    bootstrap dispatcher.
    """
    command = getattr(args, "command_module", None)
    if command is None:
        raise CommandDispatchError("Ingen command_module funnet på argparse-resultatet")

    if command.run is None:
        raise CommandDispatchError(
            f"Kommandoen '{command.name}' er ikke flyttet ut av Python-bootstrap ennå"
        )

    result = command.run(args)
    return 0 if result is None else int(result)
