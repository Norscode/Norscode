"""Diagnose command — AVVIKLA. Berre tilgjengeleg via --legacy-python-fallback. Bruk nc doctor."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.diagnostics import run_diagnostics


def register_arguments(parser) -> None:
    parser.add_argument("--path", help="Valgfri sti til prosjekt eller fil (default: nåværende mappe)")
    parser.add_argument("--json", action="store_true", help="Skriv diagnose-resultat som JSON")


def _get_payload(path: str | None = None) -> dict:
    return run_diagnostics(path=path)


def run(args) -> int:
    payload = _get_payload(path=getattr(args, "path", None))
    if getattr(args, "json", False):
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Diagnose OK: {'ja' if payload.get('ok') else 'nei'}")
        print(f"Sti: {payload['root']}")
        print(f"Prosjektrot: {payload.get('project_root') or 'fant ikke'}")
        print(f"Konfig: {payload.get('config_path') or 'fant ikke'}")
        print(
            f"Prosjekt: {payload.get('project_name') or '-'} "
            f"entry={payload.get('project_entry') or '-'}"
        )
        print(
            "Paths: "
            f"source={payload['paths'].get('source') or '-'} "
            f"stdlib={payload['paths'].get('stdlib') or '-'} "
            f"build={payload['paths'].get('build') or '-'}"
        )
        print(f"Stdlib-roots: {', '.join(payload['stdlib_roots']) if payload['stdlib_roots'] else '-'}")
        print(f"std.web resolvable: {'ja' if payload['stdlib_resolves_web'] else 'nei'}")
        print(f"Dependencies: {payload['dependency_count']}")
        print(f"Tester: {payload['test_count']}")
        print(
            "Git: "
            f"branch={payload['git'].get('branch') or '-'} "
            f"dirty={'ja' if payload['git'].get('dirty') else 'nei'} "
            f"rev={payload['git'].get('revision') or '-'}"
        )
        if payload["tests"]:
            print("Første tester:")
            for test_path in payload["tests"]:
                print(f"- {test_path}")
    return 0 if payload.get("ok") else 1


DIAGNOSE_COMMAND = CommandModule(
    name="diagnose",
    help="Vis samlet diagnose for prosjekt og runtime",
    register_arguments=register_arguments,
    run=run,
)
