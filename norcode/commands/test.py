"""Test command module.

The current implementation still delegates to the legacy Python bootstrap.
This module exists so the future self-hosted runtime can expose the same
command contract without depending on `main.py`.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule



def register_arguments(parser) -> None:
    parser.add_argument("file", nargs="?", help="Valgfri testfil")
    parser.add_argument("--verbose", action="store_true", help="Vis output også for tester som består")
    parser.add_argument("--json", action="store_true", help="Skriv testresultat som JSON")


TEST_COMMAND = CommandModule(
    name="test",
    help="Kjør én testfil eller alle i tests/",
    register_arguments=register_arguments,
)
