from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.quality_suites import run_server_e2e_suite


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv e2e-resultat som JSON")


def run(args) -> int:
    payload = run_server_e2e_suite()
    if getattr(args, "json", False):
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload['ok'] else 'nei'}")
        for row in payload["cases"]:
            status = "OK" if row.get("ok") else "FEIL"
            print(f"- {status}: {row['name']}")
    return 0 if payload["ok"] else 1


SERVE_E2E_COMMAND = CommandModule(
    name="serve-e2e",
    help="Kjør e2e-tester av serveradapteren i flere miljøer",
    register_arguments=register_arguments,
    run=run,
)
