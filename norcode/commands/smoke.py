"""Release/install smoke command."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.smoke import run_smoke_suite


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv smoke-resultat som JSON")


def run(args) -> int:
    payload = run_smoke_suite()
    if getattr(args, "json", False):
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload['ok'] else 'nei'}")
        print(f"Release: {payload['release_version']}")
        print(f"Prefix: {payload['temp_prefix']}")
        for row in payload["steps"]:
            status = "OK" if row.get("ok") else "FEIL"
            print(f"- {status}: {row['name']} ({row.get('duration_ms', 0)} ms)")
    return 0 if payload["ok"] else 1


SMOKE_COMMAND = CommandModule(
    name="smoke",
    help="Kjør fresh install/release smoke-test",
    register_arguments=register_arguments,
    run=run,
)
