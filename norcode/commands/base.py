"""Shared command abstractions for the CLI migration.

The current CLI still runs through the legacy bootstrap path.  These small
abstractions let us move commands out of `main.py` incrementally while keeping
metadata, argument registration and execution handlers together.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable, Protocol


class ParserLike(Protocol):
    def add_argument(self, *args: Any, **kwargs: Any) -> Any:
        ...


RegisterArguments = Callable[[ParserLike], None]
RunCommand = Callable[[Any], int | None]


@dataclass(frozen=True)
class CommandModule:
    name: str
    help: str
    register_arguments: RegisterArguments
    run: RunCommand | None = None
    bootstrap_only: bool = False
    experimental: bool = False
