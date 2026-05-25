from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def _run_parser_parity_fixture(path: str) -> subprocess.CompletedProcess[str]:
    repo_root = Path(__file__).resolve().parents[1]
    return subprocess.run(
        [sys.executable, "-m", "norcode", "parser-parity-fixture", path, "--check"],
        cwd=repo_root,
        text=True,
        capture_output=True,
        check=False,
    )


def test_minimal_parser_parity_fixture_is_current() -> None:
    completed = _run_parser_parity_fixture("tests/parser_parity_samples/minimal.no")
    assert completed.returncode == 0, completed.stdout + completed.stderr
    assert "Parser fixture OK" in completed.stdout


def test_import_parser_parity_fixture_is_current() -> None:
    completed = _run_parser_parity_fixture("tests/parser_parity_samples/imports.no")
    assert completed.returncode == 0, completed.stdout + completed.stderr
    assert "Parser fixture OK" in completed.stdout
