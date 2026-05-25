"""Compiler service facade for Norscode.

This module is the public runtime boundary for the migration away from the
legacy Python monolith.  It now owns the shared load/check/build/run helpers
that were previously embedded in `main.py`.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from typing import Any

from compiler.cgen import CGenerator
from compiler.lexer import Lexer
from compiler.loader import ModuleLoader
from compiler.parser import Parser
from compiler.semantic import SemanticAnalyzer


def _resolve_source_path(source_file: str) -> Path:
    path = Path(source_file).expanduser().resolve()
    if not path.exists():
        raise RuntimeError(f"Fant ikke kildefil: {path}")
    return path


def load_program(source_file: str):
    source_path = _resolve_source_path(source_file)
    loader = ModuleLoader(source_path.parent)
    loaded = loader.load_entry_file(source_path.name)

    if isinstance(loaded, tuple):
        program, alias_map = loaded
    else:
        program, alias_map = loaded, {}

    return source_path, program, alias_map


def parse_source(source_text: str):
    parser = Parser(Lexer(source_text))
    return parser.parse()


def check_program(source_file: str):
    source_path, program, alias_map = load_program(source_file)
    analyzer = SemanticAnalyzer(alias_map=alias_map)
    analyzer.analyze(program)
    return source_path, program, alias_map, analyzer


def build_program(source_file: str):
    source_path, program, alias_map, analyzer = check_program(source_file)

    cgen = CGenerator(analyzer.functions, alias_map=alias_map)
    code = cgen.generate(program)

    c_path = source_path.with_suffix(".c")
    exe_path = source_path.with_suffix("")

    c_path.write_text(code, encoding="utf-8")
    subprocess.run(["clang", str(c_path), "-lsqlite3", "-o", str(exe_path)], check=True)

    return source_path, c_path, exe_path, alias_map, analyzer


def disasm_program(source_file: str):
    source_path, program, alias_map, analyzer = check_program(source_file)
    cgen = CGenerator(analyzer.functions, alias_map=alias_map)
    code = cgen.generate(program)
    return source_path, code


def run_program(source_file: str):
    source_path, c_path, exe_path, _alias_map, _analyzer = build_program(source_file)
    print(f"Generert C-fil: {c_path}")
    print("Kompilert med: clang")
    print(f"Kjører: {exe_path}")
    result = subprocess.run([str(exe_path.resolve())], capture_output=True, text=True)
    if result.stdout:
        print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
    if result.returncode != 0:
        if result.stderr:
            print(result.stderr, end="" if result.stderr.endswith("\n") else "\n", file=sys.stderr)
        raise RuntimeError(f"Runtime-feil: kjørbar fil returnerte kode {result.returncode}")
    return source_path


def run_program_file(path: str) -> None:
    run_program(path)


def check_program_file(path: str) -> tuple[Any, Any, Any, Any]:
    return check_program(path)


def run_single_test_file(path: str, verbose: bool = False) -> dict[str, Any]:
    from norcode.testing_support import run_test_file

    return run_test_file(path)


def run_test_suite(verbose: bool = False, quiet: bool = False) -> list[dict[str, Any]]:
    from norcode.testing_support import run_all_tests

    return run_all_tests(verbose=verbose, quiet=quiet)
