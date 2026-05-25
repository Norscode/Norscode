"""Negative parser/runtime corpus used by `norcode fuzz`."""

from __future__ import annotations

import subprocess
import tempfile
import time
from pathlib import Path

from compiler.cgen import CGenerator
from compiler.lexer import Lexer
from compiler.parser import Parser
from compiler.semantic import SemanticAnalyzer


def parse_source(source_text: str):
    parser = Parser(Lexer(source_text))
    return parser.parse()


def _run_runtime_source(source_text: str) -> None:
    with tempfile.TemporaryDirectory(prefix="norscode-negative-") as tmpdir:
        tmp_path = Path(tmpdir) / "negative_runtime.no"
        c_path = tmp_path.with_suffix(".c")
        exe_path = tmp_path.with_suffix("")

        tmp_path.write_text(source_text, encoding="utf-8")
        program = parse_source(source_text)
        analyzer = SemanticAnalyzer()
        analyzer.analyze(program)
        c_code = CGenerator(analyzer.functions).generate(program)
        c_path.write_text(c_code, encoding="utf-8")
        subprocess.run(["clang", str(c_path), "-lsqlite3", "-o", str(exe_path)], check=True)
        subprocess.run([str(exe_path.resolve())], check=True, capture_output=True, text=True)


def run_negative_suite() -> dict:
    parser_cases = [
        {"name": "empty-op", "source": "funksjon start() -> heltall { + }"},
        {"name": "broken-fun", "source": "funksjon start() -> heltall { fun }"},
        {"name": "dangling-assign", "source": "funksjon start() -> heltall { la x = }"},
        {"name": "unknown-token", "source": "funksjon start() -> heltall { ??? }"},
        {"name": "missing-brace", "source": "funksjon start() -> heltall { la x = 1"},
    ]
    payload = {
        "ok": False,
        "parser_cases": [],
        "runtime_cases": [],
        "parser_failures": 0,
        "runtime_failures": 0,
    }
    for case in parser_cases:
        started = time.perf_counter()
        failed = False
        error_text = ""
        try:
            parse_source(case["source"])
        except Exception as exc:
            failed = True
            error_text = str(exc)
        duration_ms = int((time.perf_counter() - started) * 1000)
        payload["parser_cases"].append(
            {
                "name": case["name"],
                "ok": failed,
                "duration_ms": duration_ms,
                "error": error_text,
            }
        )
        if not failed:
            payload["parser_failures"] += 1

    runtime_source = "funksjon start() -> heltall { kast(\"boom\") }"
    started = time.perf_counter()
    runtime_ok = False
    runtime_error = ""
    try:
        _run_runtime_source(runtime_source)
    except subprocess.CalledProcessError as exc:
        runtime_ok = True
        runtime_error = (exc.stderr or "").strip() or (exc.stdout or "").strip()
    except Exception as exc:
        runtime_ok = True
        runtime_error = str(exc)
    duration_ms = int((time.perf_counter() - started) * 1000)
    payload["runtime_cases"].append(
        {
            "name": "unhandled-throw",
            "ok": runtime_ok,
            "duration_ms": duration_ms,
            "error": runtime_error,
        }
    )
    if not runtime_ok:
        payload["runtime_failures"] += 1

    payload["ok"] = payload["parser_failures"] == 0 and payload["runtime_failures"] == 0
    return payload
