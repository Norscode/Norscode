"""Shared lint helpers for CLI commands."""

from __future__ import annotations

import importlib


def _legacy_main():
    return importlib.import_module("main")


def lint_program(source_file: str):
    return _legacy_main().lint_program(source_file)


def print_lint_result(result, verbose: bool = False):
    return _legacy_main().print_lint_result(result, verbose=verbose)


def summarize_lint_results(result: dict) -> dict:
    return _legacy_main().summarize_lint_results(result)
