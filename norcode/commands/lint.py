"""Lint a `.no` file."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.linting import lint_program, print_lint_result, summarize_lint_results


def register_arguments(parser) -> None:
    parser.add_argument("file", help="Kildefil å lint'e")
    parser.add_argument("--verbose", action="store_true", help="Vis alle funn eksplisitt")
    parser.add_argument("--json", action="store_true", help="Skriv lint-resultat som JSON")
    parser.add_argument("--check", action="store_true", help="Feil hvis linteren finner noe")


def run(args) -> int:
    result = lint_program(args.file)
    if args.json:
        payload = {
            "mode": "single",
            "result": result,
            "summary": summarize_lint_results(result),
        }
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print_lint_result(result, verbose=args.verbose)
    if args.check and result["issues"]:
        return 1
    return 0


LINT_COMMAND = CommandModule(
    name="lint",
    help="Kjør en enkel linter på en .no-fil",
    register_arguments=register_arguments,
    run=run,
)
