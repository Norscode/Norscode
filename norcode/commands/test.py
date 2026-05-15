"""Test command module.

The command now goes through the compiler service facade instead of importing
legacy runtime helpers from `main.py` directly.
"""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.compiler_service import run_single_test_file, run_test_suite



def register_arguments(parser) -> None:
    parser.add_argument("file", nargs="?", help="Valgfri testfil")
    parser.add_argument("--verbose", action="store_true", help="Vis output også for tester som består")
    parser.add_argument("--json", action="store_true", help="Skriv testresultat som JSON")



def run(args) -> int:
    if args.file:
        result = run_single_test_file(args.file, verbose=args.verbose)
        if args.json:
            print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0 if result.get("success") else 1

    results = run_test_suite(verbose=args.verbose, quiet=args.json)
    failed = sum(1 for item in results if not item["success"])

    if args.json:
        print(json.dumps(results, ensure_ascii=False, indent=2))

    return 0 if failed == 0 else 1


TEST_COMMAND = CommandModule(
    name="test",
    help="Kjør én testfil eller alle i tests/",
    register_arguments=register_arguments,
    run=run,
)
