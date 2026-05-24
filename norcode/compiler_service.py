"""Compiler service facade for the Norscode migration.

This module is a compatibility facade around the current legacy compiler
implementation in `main.py`.  Command modules should call this service instead
of importing `main.py` directly.  That gives us one narrow place to replace the
Python implementation with a self-hosted Norscode compiler later.
"""

from __future__ import annotations

from typing import Any



def run_program_file(path: str) -> None:
    from main import run_program

    run_program(path)



def check_program_file(path: str) -> tuple[Any, Any, Any, Any]:
    from main import check_program

    return check_program(path)



def run_single_test_file(path: str, verbose: bool = False) -> dict[str, Any]:
    from main import run_test_file

    return run_test_file(path)



def run_test_suite(verbose: bool = False, quiet: bool = False) -> list[dict[str, Any]]:
    from main import run_all_tests

    return run_all_tests(verbose=verbose, quiet=quiet)
