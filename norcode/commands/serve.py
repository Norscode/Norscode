"""Serve command — Python-fallback only. Bruk nc serve (kjem snart)."""
from __future__ import annotations
from norcode.commands.base import CommandModule


def register_arguments(parser) -> None:
    parser.add_argument("file", nargs="?", default="app.no")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--debug", action="store_true")


def run(args) -> int:
    try:
        from norcode.server_runtime import serve_program
        serve_program(
            source_file=getattr(args, "file", "app.no"),
            host=getattr(args, "host", "127.0.0.1"),
            port=getattr(args, "port", 8000),
            debug=getattr(args, "debug", False),
        )
        return 0
    except ImportError:
        print("nc serve: krev --legacy-python-fallback (Python VM ikkje tilgjengeleg)")
        return 2


SERVE_COMMAND = CommandModule(
    name="serve",
    help="Køyr Norscode-webserver (krev Python)",
    register_arguments=register_arguments,
    run=run,
)
