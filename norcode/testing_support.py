# AVVIKLA: Behalde for --legacy-python-fallback. Erstatta av nc-vm.
"""Shared test-suite helpers for the modular CLI."""

from __future__ import annotations

import subprocess
import time
from pathlib import Path
from types import SimpleNamespace


def discover_tests() -> list[Path]:
    tests_dir = Path("tests").resolve()
    if not tests_dir.exists():
        return []
    return sorted(p.resolve() for p in tests_dir.rglob("test_*.no"))


def run_test_file(source_file: str):
    from norcode.compiler_service import build_program, load_program
    from compiler.semantic import SemanticAnalyzer

    started = time.perf_counter()
    source_text = Path(source_file).expanduser().resolve().read_text(encoding="utf-8")
    if "bruk selfhost." in source_text:
        from contextlib import redirect_stderr, redirect_stdout
        from io import StringIO
        from compiler.interpreter import Interpreter

        source_path, program, alias_map = load_program(source_file)
        analyzer = SemanticAnalyzer(alias_map=alias_map)
        analyzer.analyze(program)
        interpreter = Interpreter()
        stdout_buf = StringIO()
        stderr_buf = StringIO()
        try:
            with redirect_stdout(stdout_buf), redirect_stderr(stderr_buf):
                interpreter.run(program)
            returncode = 0
        except Exception as exc:
            returncode = 1
            stderr_text = stderr_buf.getvalue()
            formatted = str(exc)
            if formatted:
                stderr_text = (stderr_text + ("\n" if stderr_text and not stderr_text.endswith("\n") else "")) + formatted + "\n"
        else:
            stderr_text = stderr_buf.getvalue()
        result = SimpleNamespace(
            returncode=returncode,
            stdout=stdout_buf.getvalue(),
            stderr=stderr_text,
        )
        c_path = Path("")
        exe_path = Path("")
    else:
        source_path, c_path, exe_path, _alias_map, _analyzer = build_program(source_file)
        result = subprocess.run(
            [str(exe_path.resolve())],
            capture_output=True,
            text=True,
        )
    duration_ms = int((time.perf_counter() - started) * 1000)

    return {
        "source": str(source_path),
        "c_file": str(c_path),
        "exe_file": str(exe_path),
        "returncode": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
        "success": result.returncode == 0,
        "duration_ms": duration_ms,
    }


def print_test_result(result, verbose: bool = False):
    status = "OK" if result["success"] else "FEIL"
    duration_ms = result.get("duration_ms")
    if isinstance(duration_ms, int):
        print(f"{status}: {result['source']} ({duration_ms} ms)")
    else:
        print(f"{status}: {result['source']}")

    if verbose or not result["success"]:
        if result["stdout"]:
            print("STDOUT:")
            print(result["stdout"], end="" if result["stdout"].endswith("\n") else "\n")
        if result["stderr"]:
            print("STDERR:")
            print(result["stderr"], end="" if result["stderr"].endswith("\n") else "\n")


def run_all_tests(verbose: bool = False, quiet: bool = False):
    test_files = discover_tests()
    results = []
    ir_snapshot_result = None
    for test_file in test_files:
        result = run_test_file(str(test_file))
        results.append(result)
        if not quiet:
            print_test_result(result, verbose=verbose)

    if any("ir_snapshot" in test_file.name for test_file in test_files):
        from main import run_ir_snapshot_checks

        ir_snapshot_result = run_ir_snapshot_checks()

    if ir_snapshot_result is not None:
        results.append(ir_snapshot_result)
        if not quiet:
            print_test_result(ir_snapshot_result, verbose=verbose)

    return results
