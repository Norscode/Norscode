"""Shared command abstractions for the modular CLI.

These small abstractions keep metadata, argument registration and execution
handlers together so commands can be registered once and used by both the
modern CLI entrypoint and the legacy wrapper while the migration is still in
flight.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable, Optional, Protocol


class ParserLike(Protocol):
    def add_argument(self, *args: Any, **kwargs: Any) -> Any:
        ...


RegisterArguments = Callable[[ParserLike], None]
RunCommand = Callable[[Any], Optional[int]]


@dataclass(frozen=True)
class CommandModule:
    name: str
    help: str
    register_arguments: RegisterArguments
    run: Optional[RunCommand] = None
    bootstrap_only: bool = False
    experimental: bool = False
