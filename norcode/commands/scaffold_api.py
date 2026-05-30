"""Scaffold-api command — AVVIKLA. Berre tilgjengeleg via --legacy-python-fallback."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.scaffold import scaffold_api_project


def register_arguments(parser) -> None:
    parser.add_argument("target", help="Målmappe for prosjektet")
    parser.add_argument("--name", help="Prosjektnavn som brukes i konfig og filer")
    parser.add_argument("--force", action="store_true", help="Tillat overskriving av eksisterende filer")
    parser.add_argument("--json", action="store_true", help="Skriv scaffold-resultat som JSON")


def run(args) -> int:
    payload = scaffold_api_project(args.target, name=args.name, force=args.force)
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {payload['project_dir']}")
        print(f"Prosjekt: {payload['pretty_name']} ({payload['project_name']})")
        for row in payload["files"]:
            print(f"- {row}")
    return 0


SCAFFOLD_API_COMMAND = CommandModule(
    name="scaffold-api",
    help="Lag et nytt API-prosjekt med standard struktur",
    register_arguments=register_arguments,
    run=run,
)
