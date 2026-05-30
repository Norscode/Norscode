"""Lint command — Python-fri primærbane via nc-vm.

For Python-free linting: bin/nc lint foo.no
This Python path is only for --legacy-python-fallback.
"""
from __future__ import annotations
import os, subprocess
from norcode.commands.base import CommandModule


def register_arguments(parser) -> None:
    parser.add_argument("file", help="Kildefil å lint'e")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--json", action="store_true")


def run(args) -> int:
    # Try Python-free nc-vm path
    nc_vm = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "dist", "nc-vm")
    linter = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "selfhost", "linter.no")
    if not args.json and os.path.isfile(nc_vm) and os.path.isfile(linter) and os.access(nc_vm, os.X_OK):
        src_abs = os.path.abspath(args.file)
        env = {**os.environ, "NC_LINT_FILE": src_abs}
        if args.check:
            env["NC_LINT_CHECK"] = "1"
        result = subprocess.run([nc_vm, "--nc-run", linter], env=env)
        return result.returncode
    # Legacy Python path
    from norcode.linting import lint_program, print_lint_result, summarize_lint_results
    import json
    result = lint_program(args.file)
    if args.json:
        payload = {"mode": "single", "result": result, "summary": summarize_lint_results(result)}
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print_lint_result(result, verbose=args.verbose)
    if args.check and result["issues"]:
        return 1
    return 0


LINT_COMMAND = CommandModule(
    name="lint",
    help="Lint kjeldekode (nc-vm Python-fri, elles Python-fallback)",
    register_arguments=register_arguments,
    run=run,
)
